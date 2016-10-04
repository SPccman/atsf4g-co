#ifndef _ATFRAME_SERVICE_COMPONENT_CONFIG_EXTERN_SERVICE_TYPES_H_
#define _ATFRAME_SERVICE_COMPONENT_CONFIG_EXTERN_SERVICE_TYPES_H_

#pragma once

#include <config/atframe_service_types.h>

namespace atframe {
    namespace component {
        struct ext_service_type {
            enum type {
                EN_ATST_SS_MSG = service_type::EN_ATST_CUSTOM_START, // solution services
            };
        };
    }
}
#endif