// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/url_request/url_request_context.h"

#include <inttypes.h>
#include <stdint.h>

#include "base/compiler_specific.h"
#include "base/debug/alias.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/chromeos_buildflags.h"
#include "net/base/http_user_agent_settings.h"
#include "net/cookies/cookie_store.h"
#include "net/dns/host_resolver.h"
#include "net/http/http_cache.h"
#include "net/http/http_network_session.h"
#include "net/http/http_transaction_factory.h"
#include "net/socket/ssl_client_socket_impl.h"
#include "net/url_request/url_request.h"

namespace net {

URLRequestContext::URLRequestContext()
    : net_log_(nullptr),
      host_resolver_(nullptr),
      cert_verifier_(nullptr),
      http_auth_handler_factory_(nullptr),
      proxy_resolution_service_(nullptr),
      proxy_delegate_(nullptr),
      ssl_config_service_(nullptr),
      network_delegate_(nullptr),
      http_server_properties_(nullptr),
      http_user_agent_settings_(nullptr),
      cookie_store_(nullptr),
      transport_security_state_(nullptr),
      ct_policy_enforcer_(nullptr),
      sct_auditing_delegate_(nullptr),
      http_transaction_factory_(nullptr),
      job_factory_(nullptr),
      throttler_manager_(nullptr),
      quic_context_(nullptr),
      network_quality_estimator_(nullptr),
#if BUILDFLAG(ENABLE_REPORTING)
      reporting_service_(nullptr),
      network_error_logging_service_(nullptr),
#endif  // BUILDFLAG(ENABLE_REPORTING)
      url_requests_(std::make_unique<std::set<const URLRequest*>>()),
      enable_brotli_(false),
      check_cleartext_permitted_(false),
      require_network_isolation_key_(false) {
}

URLRequestContext::~URLRequestContext() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  AssertNoURLRequests();
}

const HttpNetworkSessionParams* URLRequestContext::GetNetworkSessionParams()
    const {
  HttpTransactionFactory* transaction_factory = http_transaction_factory();
  if (!transaction_factory)
    return nullptr;
  HttpNetworkSession* network_session = transaction_factory->GetSession();
  if (!network_session)
    return nullptr;
  return &network_session->params();
}

const HttpNetworkSessionContext* URLRequestContext::GetNetworkSessionContext()
    const {
  HttpTransactionFactory* transaction_factory = http_transaction_factory();
  if (!transaction_factory)
    return nullptr;
  HttpNetworkSession* network_session = transaction_factory->GetSession();
  if (!network_session)
    return nullptr;
  return &network_session->context();
}

// TODO(crbug.com/1052397): Revisit once build flag switch of lacros-chrome is
// complete.
#if !defined(OS_WIN) && !(defined(OS_LINUX) || BUILDFLAG(IS_CHROMEOS_LACROS))
std::unique_ptr<URLRequest> URLRequestContext::CreateRequest(
    const GURL& url,
    RequestPriority priority,
    URLRequest::Delegate* delegate) const {
  return CreateRequest(url, priority, delegate, MISSING_TRAFFIC_ANNOTATION,
                       /*is_for_websockets=*/false);
}
#endif

std::unique_ptr<URLRequest> URLRequestContext::CreateRequest(
    const GURL& url,
    RequestPriority priority,
    URLRequest::Delegate* delegate,
    NetworkTrafficAnnotationTag traffic_annotation,
    bool is_for_websockets,
    const absl::optional<uint32_t> net_log_source_id) const {
  return base::WrapUnique(new URLRequest(url, priority, delegate, this,
                                         traffic_annotation, is_for_websockets,
                                         net_log_source_id));
}

void URLRequestContext::set_cookie_store(CookieStore* cookie_store) {
  cookie_store_ = cookie_store;
}

void URLRequestContext::AssertNoURLRequests() const {
  int num_requests = url_requests_->size();
  if (num_requests != 0) {
    // We're leaking URLRequests :( Dump the URL of the first one and record how
    // many we leaked so we have an idea of how bad it is.
    const URLRequest* request = *url_requests_->begin();
    int load_flags = request->load_flags();
    DEBUG_ALIAS_FOR_GURL(url_buf, request->url());
    base::debug::Alias(&num_requests);
    base::debug::Alias(&load_flags);
    CHECK(false) << "Leaked " << num_requests << " URLRequest(s). First URL: "
                 << request->url().spec().c_str() << ".";
  }
}

}  // namespace net
