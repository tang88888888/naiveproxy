// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_DNS_DNS_CONFIG_SERVICE_WIN_H_
#define NET_DNS_DNS_CONFIG_SERVICE_WIN_H_

// The sole purpose of dns_config_service_win.h is for unittests so we just
// include these headers here.
#include <winsock2.h>
#include <iphlpapi.h>
#include <iptypes.h>

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/free_deleter.h"
#include "base/memory/ref_counted.h"
#include "base/strings/string_piece_forward.h"
#include "net/base/net_export.h"
#include "net/dns/dns_config_service.h"
#include "net/dns/public/win_dns_system_settings.h"

// The general effort of DnsConfigServiceWin is to configure |nameservers| and
// |search| in DnsConfig. The settings are stored in the Windows registry, but
// to simplify the task we use the IP Helper API wherever possible. That API
// yields the complete and ordered |nameservers|, but to determine |search| we
// need to use the registry. On Windows 7, WMI does return the correct |search|
// but on earlier versions it is insufficient.
//
// Experimental evaluation of Windows behavior suggests that domain parsing is
// naive. Domain suffixes in |search| are not validated until they are appended
// to the resolved name. We attempt to replicate this behavior.

namespace net {

namespace internal {

// Converts a UTF-16 domain name to ASCII, possibly using punycode.
// Returns empty string on failure.
std::string NET_EXPORT_PRIVATE ParseDomainASCII(base::WStringPiece widestr);

// Parses |value| as search list (comma-delimited list of domain names) from
// a registry key and stores it in |out|. Returns empty vector on failure. Empty
// entries (e.g., "chromium.org,,org") terminate the list. Non-ascii hostnames
// are converted to punycode.
std::vector<std::string> NET_EXPORT_PRIVATE
ParseSearchList(base::WStringPiece value);

// Fills in |dns_config| from |settings|. Exposed for tests. Returns nullopt if
// a valid config could not be determined.
absl::optional<DnsConfig> NET_EXPORT_PRIVATE
ConvertSettingsToDnsConfig(const WinDnsSystemSettings& settings);

// Service for reading and watching Windows system DNS settings. This object is
// not thread-safe and methods may perform blocking I/O so methods must be
// called on a sequence that allows blocking (i.e. base::MayBlock). It may be
// constructed on a different sequence than which it's later called on.
// WatchConfig() must be called prior to ReadConfig().
// Use DnsConfigService::CreateSystemService to use it outside of tests.
class NET_EXPORT_PRIVATE DnsConfigServiceWin : public DnsConfigService {
 public:
  DnsConfigServiceWin();

  DnsConfigServiceWin(const DnsConfigServiceWin&) = delete;
  DnsConfigServiceWin& operator=(const DnsConfigServiceWin&) = delete;

  ~DnsConfigServiceWin() override;

 private:
  class Watcher;
  class ConfigReader;
  class HostsReader;

  // DnsConfigService:
  void ReadConfigNow() override;
  void ReadHostsNow() override;
  bool StartWatching() override;

  std::unique_ptr<Watcher> watcher_;
  scoped_refptr<ConfigReader> config_reader_;
  scoped_refptr<HostsReader> hosts_reader_;
};

}  // namespace internal

}  // namespace net

#endif  // NET_DNS_DNS_CONFIG_SERVICE_WIN_H_
