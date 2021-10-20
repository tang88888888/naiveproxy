// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_DNS_PUBLIC_RESOLV_READER_H_
#define NET_DNS_PUBLIC_RESOLV_READER_H_

#include <resolv.h>

#include <memory>
#include <vector>

#include "net/base/ip_endpoint.h"
#include "net/base/net_export.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace net {

// Test-overridable class to handle the interactions with OS APIs for reading
// resolv.conf.
class NET_EXPORT ResolvReader {
 public:
  virtual ~ResolvReader() = default;

  // Null on failure. If not null, result must be cleaned up through a call
  // to `CloseResState()`.
  virtual std::unique_ptr<struct __res_state> GetResState();
  virtual void CloseResState(struct __res_state* res);
};

// Returns configured DNS servers or nullopt on failure.
NET_EXPORT absl::optional<std::vector<IPEndPoint>> GetNameservers(
    const struct __res_state& res);

}  // namespace net

#endif  // NET_DNS_PUBLIC_RESOLV_READER_H_