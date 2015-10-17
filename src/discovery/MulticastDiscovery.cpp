/* Copyright (C) 2015 Alexander Shishenko <GamePad64@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "MulticastDiscovery.h"
#include "MulticastDiscovery.pb.h"
#include "../Session.h"
#include "../directory/ExchangeGroup.h"
#include "../directory/p2p/P2PProvider.h"
#include "../directory/Exchanger.h"

namespace librevault {

using namespace boost::asio::ip;

/* MulticastSender */
MulticastSender::MulticastSender(MulticastDiscovery& parent, std::shared_ptr<ExchangeGroup> exchange_group) :
		parent_(parent), exchange_group_(exchange_group), repeat_timer_(parent_.session_.ios()), repeat_interval_(parent.repeat_interval_) {
	send();
}

std::string MulticastSender::get_message() const {
	if(message_.empty()){
		protocol::MulticastDiscovery message;
		message.set_port(parse_url(parent_.session_.config().get<std::string>("net.listen")).port);
		message.set_dir_hash(exchange_group_->key().get_Hash().data(), exchange_group_->key().get_Hash().size());
		message.set_pubkey(parent_.exchanger_.get_p2p_provider()->node_key().public_key().data(), parent_.exchanger_.get_p2p_provider()->node_key().public_key().size());

		message_ = message.SerializeAsString();
	}
	return message_;
}

void MulticastSender::wait(){
	repeat_timer_.expires_from_now(repeat_interval_);
	repeat_timer_.async_wait(std::bind(&MulticastSender::send, this));
}

void MulticastSender::send(){
	parent_.socket_.async_send_to(boost::asio::buffer(get_message()), parent_.multicast_addr_, std::bind(&MulticastSender::wait, this));
	parent_.log_->debug() << parent_.log_tag() << "==> " << parent_.multicast_addr_;
}

/* MulticastDiscovery */
MulticastDiscovery::MulticastDiscovery(Session& session, Exchanger& exchanger, ptree& options) :
		DiscoveryService(session, exchanger), local_options_(options), socket_(session.ios()) {

	bind_address_ = address::from_string(local_options_.get<std::string>("local_ip"));

	repeat_interval_ = std::chrono::seconds(local_options_.get<int64_t>("repeat_interval"));
	multicast_addr_.port(local_options_.get<uint16_t>("port"));
	multicast_addr_.address(address::from_string(local_options_.get<std::string>("ip")));
}

MulticastDiscovery::~MulticastDiscovery() {
	socket_.set_option(multicast::leave_group(multicast_addr_.address()));
}

void MulticastDiscovery::register_group(std::shared_ptr<ExchangeGroup> group_ptr) {
	senders_.insert({group_ptr, std::make_shared<MulticastSender>(*this, group_ptr)});
}

void MulticastDiscovery::unregister_group(std::shared_ptr<ExchangeGroup> group_ptr) {
	senders_.erase(group_ptr);
}

void MulticastDiscovery::start(){
	socket_.set_option(multicast::join_group(multicast_addr_.address()));
	socket_.set_option(multicast::enable_loopback(false));
	socket_.set_option(udp::socket::reuse_address(true));

	socket_.bind(udp::endpoint(bind_address_, multicast_addr_.port()));

	receive();

	log_->info() << log_tag() << "Started UDP Local Node Discovery on: " << multicast_addr_;
}

void MulticastDiscovery::process(std::shared_ptr<udp_buffer> buffer, size_t size, std::shared_ptr<udp::endpoint> endpoint_ptr){
	protocol::MulticastDiscovery message;
	if(message.ParseFromArray(buffer->data(), size)){
		uint16_t port = message.port();
		blob dir_hash = blob(message.dir_hash().begin(), message.dir_hash().end());
		blob pubkey = blob(message.pubkey().begin(), message.pubkey().end());

		std::shared_ptr<ExchangeGroup> group_ptr;
		for(auto& sender_it : senders_){
			if(sender_it.first->hash() == dir_hash)
				group_ptr = sender_it.first; break;
		}

		if(group_ptr){
			tcp_endpoint node_endpoint(endpoint_ptr->address(), port);
			log_->debug() << log_tag() << "<== " << node_endpoint;
			add_node(node_endpoint, pubkey, group_ptr);
		}
	}else{
		log_->debug() << log_tag() << "Message from " << endpoint_ptr->address() << ": Malformed Protobuf data";
	}

	receive();	// We received message, continue receiving others
}

void MulticastDiscovery::receive(){
	auto endpoint = std::make_shared<udp::endpoint>(socket_.local_endpoint());
	auto buffer = std::make_shared<udp_buffer>();
	socket_.async_receive_from(boost::asio::buffer(buffer->data(), buffer->size()), *endpoint,
							  std::bind(&MulticastDiscovery::process, this, buffer, std::placeholders::_2, endpoint));
}

MulticastDiscovery4::MulticastDiscovery4(Session& session, Exchanger& exchanger) :
		MulticastDiscovery(session, exchanger, session.config().get_child("discovery.multicast4")) {
	socket_.open(boost::asio::ip::udp::v4());
	start();
}

MulticastDiscovery6::MulticastDiscovery6(Session& session, Exchanger& exchanger) :
		MulticastDiscovery(session, exchanger, session.config().get_child("discovery.multicast6")) {
	socket_.open(boost::asio::ip::udp::v6());
	socket_.set_option(boost::asio::ip::v6_only(true));
	start();
}

} /* namespace librevault */