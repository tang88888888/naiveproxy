// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/dns/mdns_client.h"

#include "net/base/address_family.h"
#include "net/base/net_errors.h"
#include "net/base/network_interfaces.h"
#include "net/dns/mdns_client_impl.h"
#include "net/dns/public/util.h"
#include "net/log/net_log.h"
#include "net/log/net_log_source.h"

namespace net {

namespace {

int Bind(AddressFamily address_family,
         uint32_t interface_index,
         DatagramServerSocket* socket) {
  socket->AllowAddressSharingForMulticast();
  socket->SetMulticastInterface(interface_index);

  int rv = socket->Listen(dns_util::GetMdnsReceiveEndPoint(address_family));
  if (rv < OK)
    return rv;

  return socket->JoinGroup(
      dns_util::GetMdnsGroupEndPoint(address_family).address());
}

}  // namespace

const base::TimeDelta MDnsTransaction::kTransactionTimeout =
    base::TimeDelta::FromSeconds(3);

// static
std::unique_ptr<MDnsSocketFactory> MDnsSocketFactory::CreateDefault() {
  return std::unique_ptr<MDnsSocketFactory>(new MDnsSocketFactoryImpl);
}

// static
std::unique_ptr<MDnsClient> MDnsClient::CreateDefault() {
  return std::unique_ptr<MDnsClient>(new MDnsClientImpl());
}

InterfaceIndexFamilyList GetMDnsInterfacesToBind() {
  NetworkInterfaceList network_list;
  InterfaceIndexFamilyList interfaces;
  if (!GetNetworkList(&network_list, INCLUDE_HOST_SCOPE_VIRTUAL_INTERFACES))
    return interfaces;
  for (size_t i = 0; i < network_list.size(); ++i) {
    AddressFamily family = GetAddressFamily(network_list[i].address);
    if (family == ADDRESS_FAMILY_IPV4 || family == ADDRESS_FAMILY_IPV6) {
      interfaces.push_back(
          std::make_pair(network_list[i].interface_index, family));
    }
  }
  std::sort(interfaces.begin(), interfaces.end());
  // Interfaces could have multiple addresses. Filter out duplicate entries.
  interfaces.erase(std::unique(interfaces.begin(), interfaces.end()),
                   interfaces.end());
  return interfaces;
}

std::unique_ptr<DatagramServerSocket> CreateAndBindMDnsSocket(
    AddressFamily address_family,
    uint32_t interface_index,
    NetLog* net_log) {
  std::unique_ptr<DatagramServerSocket> socket(
      new UDPServerSocket(net_log, NetLogSource()));

  int rv = Bind(address_family, interface_index, socket.get());
  if (rv != OK) {
    socket.reset();
    VLOG(1) << "MDNS bind failed, address_family=" << address_family
            << ", error=" << rv;
  }
  return socket;
}

}  // namespace net
