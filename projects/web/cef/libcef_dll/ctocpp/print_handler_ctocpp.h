// Copyright (c) 2017 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.
//
// ---------------------------------------------------------------------------
//
// This file was generated by the CEF translator tool. If making changes by
// hand only do so within the body of existing method and function
// implementations. See the translator.README.txt file in the tools directory
// for more information.
//
// $hash=4a628ee393964285a7ad6f2a6c64cff8a88c3b20$
//

#ifndef CEF_LIBCEF_DLL_CTOCPP_PRINT_HANDLER_CTOCPP_H_
#define CEF_LIBCEF_DLL_CTOCPP_PRINT_HANDLER_CTOCPP_H_
#pragma once

#if !defined(BUILDING_CEF_SHARED)
#error This file can be included DLL-side only
#endif

#include "include/capi/cef_print_handler_capi.h"
#include "include/cef_print_handler.h"
#include "libcef_dll/ctocpp/ctocpp_ref_counted.h"

// Wrap a C structure with a C++ class.
// This class may be instantiated and accessed DLL-side only.
class CefPrintHandlerCToCpp : public CefCToCppRefCounted<CefPrintHandlerCToCpp,
                                                         CefPrintHandler,
                                                         cef_print_handler_t> {
 public:
  CefPrintHandlerCToCpp();

  // CefPrintHandler methods.
  void OnPrintStart(CefRefPtr<CefBrowser> browser) override;
  void OnPrintSettings(CefRefPtr<CefBrowser> browser,
                       CefRefPtr<CefPrintSettings> settings,
                       bool get_defaults) override;
  bool OnPrintDialog(CefRefPtr<CefBrowser> browser,
                     bool has_selection,
                     CefRefPtr<CefPrintDialogCallback> callback) override;
  bool OnPrintJob(CefRefPtr<CefBrowser> browser,
                  const CefString& document_name,
                  const CefString& pdf_file_path,
                  CefRefPtr<CefPrintJobCallback> callback) override;
  void OnPrintReset(CefRefPtr<CefBrowser> browser) override;
  CefSize GetPdfPaperSize(int device_units_per_inch) override;
};

#endif  // CEF_LIBCEF_DLL_CTOCPP_PRINT_HANDLER_CTOCPP_H_
