/** @file
    @brief Header

    @date 2015

    @author
    Sensics, Inc.
    <http://sensics.com/osvr>
*/

// Copyright 2015 Sensics, Inc.
//
// All rights reserved.
//
// (Final version intended to be licensed under
// the Apache License, Version 2.0)

#ifndef INCLUDED_Imaging_h_GUID_5473F500_E901_419D_46D2_62ED5CB57412
#define INCLUDED_Imaging_h_GUID_5473F500_E901_419D_46D2_62ED5CB57412

// Internal Includes
#include <osvr/ClientKit/Imaging_decl.h>
#include <osvr/ClientKit/Interface.h>
#include <osvr/ClientKit/ImagingC.h>
#include <osvr/Util/ImagingReportTypesC.h>
#include <osvr/Util/TimeValue.h>
#include <osvr/Util/OpenCVTypeDispatch.h>
#include <osvr/Util/Deletable.h>

// Library/third-party includes
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <opencv2/core/core.hpp>

// Standard includes
// - none

namespace osvr {

namespace clientkit {
    void registerImagingCallback(Interface &iface, ImagingCallbackOpenCV cb,
                                 void *userdata);
    /// @brief Implementation details
    namespace detail {
        /// @brief Class serving to maintain the registration of and wrap a
        /// friendly imaging callback
        class ImagingCallbackRegistration : public util::Deletable,
                                            boost::noncopyable {
          public:
            virtual ~ImagingCallbackRegistration() {}

          private:
            /// @brief Constructor - private so that only the factory
            /// (registration) function can create it.
            ImagingCallbackRegistration(osvr::clientkit::Interface iface,
                                        ImagingCallbackOpenCV cb,
                                        void *userdata)
                : m_cb(cb), m_userdata(userdata),
                  m_ctx(iface.getContext().get()) {
                osvrRegisterImagingCallback(
                    iface.get(),
                    &ImagingCallbackRegistration::handleRawImagingCallback,
                    this);
            }
            /// @brief Custom deleter for shared pointer to image buffer
            class ImagingDeleter {
              public:
                ImagingDeleter(OSVR_ClientContext ctx) : m_ctx(ctx) {}
                void operator()(OSVR_ImageBufferElement *buf) {
                    osvrClientFreeImage(m_ctx, buf);
                }

              private:
                OSVR_ClientContext m_ctx;
            };

            /// @brief Raw callback that processes the data and calls the user's
            /// callback.
            static void
            handleRawImagingCallback(void *userdata,
                                     const struct OSVR_TimeValue *timestamp,
                                     const struct OSVR_ImagingReport *report) {
                ImagingCallbackRegistration *self =
                    static_cast<ImagingCallbackRegistration *>(userdata);
                ImagingReportOpenCV newReport;
                newReport.sensor = report->sensor;
                newReport.buffer.reset(report->state.data,
                                       ImagingDeleter(self->m_ctx));
                newReport.frame = cv::Mat(
                    report->state.metadata.height, report->state.metadata.width,
                    util::computeOpenCVMatType(report->state.metadata),
                    newReport.buffer.get());
                self->m_cb(self->m_userdata, *timestamp, newReport);
            }

            ImagingCallbackOpenCV m_cb;
            void *m_userdata;
            OSVR_ClientContext m_ctx;
            friend void osvr::clientkit::registerImagingCallback(
                Interface &iface, ImagingCallbackOpenCV cb, void *userdata);
        };
    } // namespace detail

    inline void registerImagingCallback(Interface &iface,
                                        ImagingCallbackOpenCV cb,
                                        void *userdata) {
        util::boost_util::DeletablePtr ptr(
            new detail::ImagingCallbackRegistration(iface, cb, userdata));
        iface.takeOwnership(ptr);
    }

} // end namespace clientkit

} // end namespace osvr

#endif // INCLUDED_Imaging_h_GUID_5473F500_E901_419D_46D2_62ED5CB57412