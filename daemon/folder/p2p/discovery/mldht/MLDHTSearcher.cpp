/* Copyright (C) 2015 Alexander Shishenko <GamePad64@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "MLDHTSearcher.h"
#include "MLDHTDiscovery.h"
#include "Client.h"
#include "folder/FolderGroup.h"
#include "folder/p2p/P2PProvider.h"
#include "folder/p2p/P2PFolder.h"
#include "folder/p2p/WSServer.h"
#include <dht.h>

namespace librevault {

using namespace boost::asio::ip;

MLDHTSearcher::MLDHTSearcher(std::weak_ptr<FolderGroup> group, MLDHTDiscovery& service) :
		DiscoveryInstance(group, service),
		Loggable("MLDHTSearcher"),
		announce_timer6_(service.client().network_ios()),
		search_timer6_(service.client().network_ios()),
		announce_timer4_(service.client().network_ios()),
		search_timer4_(service.client().network_ios()) {
	info_hash_ = btcompat::get_info_hash(std::shared_ptr<FolderGroup>(group)->hash());

	set_enabled(std::shared_ptr<FolderGroup>(group)->params().mainline_dht_enabled);

	attached_connection_ = std::shared_ptr<FolderGroup>(group)->attached_signal.connect([&, this](std::shared_ptr<P2PFolder> p2p_folder){
		if(enabled_) {
			dht_ping_node(p2p_folder->remote_endpoint().data(), p2p_folder->remote_endpoint().size());
			log_->debug() << log_tag() << "Added a new DHT node: " << p2p_folder->remote_endpoint();
		}
	});
}

void MLDHTSearcher::set_enabled(bool enable) {
	if(enable && !enabled_) {
		enabled_ = true;
		service_.client().network_ios().post([&, this]() {
			start_search(AF_INET, true);
			start_search(AF_INET6, true);
			start_search(AF_INET, false);
			start_search(AF_INET6, false);
		});
	}else if(!enable && enabled_) {
		enabled_ = false;

		announce_timer6_.cancel();
		search_timer6_.cancel();
		announce_timer4_.cancel();
		search_timer4_.cancel();
	}
}

void MLDHTSearcher::start_search(int af, bool announce) {
	if(enabled_) {
		bool ready6 = (af == AF_INET6 && ((MLDHTDiscovery&)service_).active_v6());
		bool ready4 = (af == AF_INET && ((MLDHTDiscovery&)service_).active_v4());

		if(ready6 || ready4) {
			log_->debug() << log_tag() << "Starting " << (ready6 ? "IPv6" : (ready4 ? "IPv4" : "")) << " search for: " << crypto::Hex().to_string(info_hash_);

			lv_dht_closure* closure = new lv_dht_closure();
			closure->discovery_ptr = (MLDHTDiscovery*)&service_;
			dht_search(info_hash_.data(), announce ? service_.client().p2p_provider()->public_port() : 0, af, lv_dht_callback_glue, closure);
		}
	}

	boost::asio::steady_timer* timer;

	if(announce) {
		if(af == AF_INET6)
			timer = &announce_timer6_;
		else
			timer = &announce_timer4_;

		timer->expires_from_now(std::chrono::minutes(5));
	}else{
		if(af == AF_INET6)
			timer = &search_timer6_;
		else
			timer = &search_timer4_;

		timer->expires_from_now(std::chrono::minutes(1));
	}

	timer->async_wait(std::bind([this](int af, bool announce, const boost::system::error_code& error){
		if(error == boost::asio::error::operation_aborted) return;
		start_search(af, announce);
	}, af, announce, std::placeholders::_1));
}

void MLDHTSearcher::search_completed(bool start_v4, bool start_v6) {

}

} /* namespace librevault */
