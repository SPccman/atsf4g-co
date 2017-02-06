//
// Created by owt50 on 2017/2/6.
//

#include <common/string_oprs.h>

#include "protobuf_mini_dumper.h"

#define MSG_DISPATCHER_DEBUG_PRINT_BOUND 4096

const char* protobuf_mini_dumper_get_readable(const ::google::protobuf::Message& msg) {
    static char msg_buffer[MSG_DISPATCHER_DEBUG_PRINT_BOUND] = {0};

    msg_buffer[0] = 0;
    size_t sz = protobuf_mini_dumper_dump_readable(msg, msg_buffer, MSG_DISPATCHER_DEBUG_PRINT_BOUND - 1, 0);

    if (sz > MSG_DISPATCHER_DEBUG_PRINT_BOUND - 5) {
        msg_buffer[MSG_DISPATCHER_DEBUG_PRINT_BOUND - 5] = '.';
        msg_buffer[MSG_DISPATCHER_DEBUG_PRINT_BOUND - 4] = '.';
        msg_buffer[MSG_DISPATCHER_DEBUG_PRINT_BOUND - 3] = '.';
        msg_buffer[MSG_DISPATCHER_DEBUG_PRINT_BOUND - 2] = '}';
        msg_buffer[MSG_DISPATCHER_DEBUG_PRINT_BOUND - 1] = 0;
    }
    return msg_buffer;
}

size_t protobuf_mini_dumper_dump_readable(const ::google::protobuf::Message& msg, char* buf, size_t bufsz, int ident) {
    std::vector<const ::google::protobuf::FieldDescriptor*> fds;
    const ::google::protobuf::Reflection* reflect = msg.GetReflection();
    if (NULL == reflect || bufsz <= 1) {
        return bufsz;
    }

    reflect->ListFields(msg, &fds);

    // 小量field的message走精简模式
    bool short_mode = fds.size() < 10;
    for (size_t i = 0; short_mode && i < fds.size(); ++ i) {
        // 有message或者repeated字段则不能是精简模式
        if (fds[i]->is_repeated() ||
            ::google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE == fds[i]->cpp_type()) {
            short_mode = false;
            break;
        }
    }

    if (!short_mode) {
        ++ident;
    }

    // dump 数据
    size_t sz = 0;
    buf[sz ++] = '{';

    for (size_t i = 0; i < fds.size() && sz < bufsz; ++ i) {
        if (short_mode) {
            buf[sz ++] = ' ';
        } else {
            buf[sz ++] = '\n';
        }

        if (sz >= bufsz) {
            break;
        }

        // 缩进
        if (!short_mode) {
            for (int j = 0; j < ident; ++ j) {
                if (sz < bufsz - 1) {
                    buf[sz ++] = ' ';
                    buf[sz ++] = ' ';
                } else {
                    break;
                }
            }
        }

        if (sz >= bufsz) {
            break;
        }

        // 字段名
        {
            int res = UTIL_STRFUNC_SNPRINTF(&buf[sz], bufsz - sz, "%s: ", fds[i]->name().c_str());
            if (res > 0) {
                sz += static_cast<size_t>(res);
            } else {
                break;
            }
        }

        if (fds[i]->is_repeated()) {
            buf[sz ++] = '[';
            int repeated_sz = reflect->FieldSize(msg, fds[i]);

            switch(fds[i]->cpp_type()) {
                case google::protobuf::FieldDescriptor::CPPTYPE_INT32: {
                    for (int j = 0; j < repeated_sz; ++ j) {
                        if (sz < bufsz) {
                            buf[sz ++] = ' ';
                        }

                        int res = UTIL_STRFUNC_SNPRINTF(&buf[sz], bufsz - sz, "%d", static_cast<int>(reflect->GetRepeatedInt32(msg, fds[i], j)));
                        if (res > 0) {
                            sz += static_cast<size_t>(res);
                        } else {
                            break;
                        }

                        if (sz < bufsz && j != repeated_sz - 1) {
                            buf[sz ++] = ',';
                        }
                    }
                    break;
                }
                case google::protobuf::FieldDescriptor::CPPTYPE_INT64:{
                    for (int j = 0; j < repeated_sz; ++ j) {
                        if (sz < bufsz) {
                            buf[sz ++] = ' ';
                        }

                        int res = UTIL_STRFUNC_SNPRINTF(&buf[sz], bufsz - sz, "%lld", static_cast<long long>(reflect->GetRepeatedInt64(msg, fds[i], j)));
                        if (res > 0) {
                            sz += static_cast<size_t>(res);
                        } else {
                            break;
                        }

                        if (sz < bufsz && j != repeated_sz - 1) {
                            buf[sz ++] = ',';
                        }
                    }
                    break;
                }
                case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:{
                    for (int j = 0; j < repeated_sz; ++ j) {
                        if (sz < bufsz) {
                            buf[sz ++] = ' ';
                        }

                        int res = UTIL_STRFUNC_SNPRINTF(&buf[sz], bufsz - sz, "%u", static_cast<unsigned int>(reflect->GetRepeatedUInt32(msg, fds[i], j)));
                        if (res > 0) {
                            sz += static_cast<size_t>(res);
                        } else {
                            break;
                        }

                        if (sz < bufsz && j != repeated_sz - 1) {
                            buf[sz ++] = ',';
                        }
                    }
                    break;
                }
                case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:{
                    for (int j = 0; j < repeated_sz; ++ j) {
                        if (sz < bufsz) {
                            buf[sz ++] = ' ';
                        }

                        int res = UTIL_STRFUNC_SNPRINTF(&buf[sz], bufsz - sz, "%llu", static_cast<unsigned long long>(reflect->GetRepeatedUInt64(msg, fds[i], j)));
                        if (res > 0) {
                            sz += static_cast<size_t>(res);
                        } else {
                            break;
                        }

                        if (sz < bufsz && j != repeated_sz - 1) {
                            buf[sz ++] = ',';
                        }
                    }
                    break;
                }
                case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:{
                    for (int j = 0; j < repeated_sz; ++ j) {
                        if (sz < bufsz) {
                            buf[sz ++] = ' ';
                        }

                        int res = UTIL_STRFUNC_SNPRINTF(&buf[sz], bufsz - sz, "%lg", static_cast<double>(reflect->GetRepeatedDouble(msg, fds[i], j)));
                        if (res > 0) {
                            sz += static_cast<size_t>(res);
                        } else {
                            break;
                        }

                        if (sz < bufsz && j != repeated_sz - 1) {
                            buf[sz ++] = ',';
                        }
                    }
                    break;
                }
                case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:{
                    for (int j = 0; j < repeated_sz; ++ j) {
                        if (sz < bufsz) {
                            buf[sz ++] = ' ';
                        }

                        int res = UTIL_STRFUNC_SNPRINTF(&buf[sz], bufsz - sz, "%g", static_cast<float>(reflect->GetRepeatedFloat(msg, fds[i], j)));
                        if (res > 0) {
                            sz += static_cast<size_t>(res);
                        } else {
                            break;
                        }

                        if (sz < bufsz && j != repeated_sz - 1) {
                            buf[sz ++] = ',';
                        }
                    }
                    break;
                }
                case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:{
                    for (int j = 0; j < repeated_sz; ++ j) {
                        if (sz < bufsz) {
                            buf[sz ++] = ' ';
                        }

                        int res;
                        if (reflect->GetRepeatedBool(msg, fds[i], j)){
                            res = UTIL_STRFUNC_SNPRINTF(&buf[sz], bufsz - sz, "%s", "true");
                        } else {
                            res = UTIL_STRFUNC_SNPRINTF(&buf[sz], bufsz - sz, "%s", "false");
                        }
                        if (res > 0) {
                            sz += static_cast<size_t>(res);
                        } else {
                            break;
                        }

                        if (sz < bufsz && j != repeated_sz - 1) {
                            buf[sz ++] = ',';
                        }
                    }
                    break;
                }
                case google::protobuf::FieldDescriptor::CPPTYPE_ENUM: {
                    for (int j = 0; j < repeated_sz; ++ j) {
                        if (sz < bufsz) {
                            buf[sz ++] = ' ';
                        }

                        const ::google::protobuf::EnumValueDescriptor* val = reflect->GetRepeatedEnum(msg, fds[i], j);
                        int res = UTIL_STRFUNC_SNPRINTF(&buf[sz], bufsz - sz, "%s", val->name().c_str());
                        if (res > 0) {
                            sz += static_cast<size_t>(res);
                        } else {
                            break;
                        }

                        if (sz < bufsz && j != repeated_sz - 1) {
                            buf[sz ++] = ',';
                        }
                    }
                    break;
                }

                case google::protobuf::FieldDescriptor::CPPTYPE_STRING: {
                    for (int j = 0; j < repeated_sz; ++ j) {
                        if (sz < bufsz) {
                            buf[sz ++] = ' ';
                        }

                        std::string default_val;
                        const std::string& val = reflect->GetRepeatedStringReference(msg, fds[i], j, &default_val);
                        int res = 0;
                        if (google::protobuf::FieldDescriptor::TYPE_BYTES != fds[i]->type()) {
                            res = UTIL_STRFUNC_SNPRINTF(&buf[sz], bufsz - sz, "%s", val.c_str());
                        } else {
                            res = UTIL_STRFUNC_SNPRINTF(&buf[sz], bufsz - sz, "BYTES(%llu)", static_cast<unsigned long long>(val.size()));
                        }

                        if (res > 0) {
                            sz += static_cast<size_t>(res);
                        } else {
                            break;
                        }

                        if (sz < bufsz && j != repeated_sz - 1) {
                            buf[sz ++] = ',';
                        }
                    }
                    break;
                }

                case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:{
                    for (int j = 0; j < repeated_sz; ++ j) {
                        if (sz < bufsz) {
                            buf[sz ++] = ' ';
                        }

                        sz += protobuf_mini_dumper_dump_readable(reflect->GetRepeatedMessage(msg, fds[i], j), &buf[sz], bufsz - sz, ident);
                        if (sz < bufsz && j != repeated_sz - 1) {
                            buf[sz ++] = ',';
                        }
                    }
                    break;
                }
            }

            if (sz < bufsz) {
                buf[sz ++] = ' ';
            }

            if (sz < bufsz) {
                buf[sz ++] = ']';
            } else {
                break;
            }
        } else {
            switch(fds[i]->cpp_type()) {
                case google::protobuf::FieldDescriptor::CPPTYPE_INT32: {
                    int res = UTIL_STRFUNC_SNPRINTF(&buf[sz], bufsz - sz, "%d", static_cast<int>(reflect->GetInt32(msg, fds[i])));
                    if (res > 0) {
                        sz += static_cast<size_t>(res);
                    }
                    break;
                }
                case google::protobuf::FieldDescriptor::CPPTYPE_INT64:{
                    int res = UTIL_STRFUNC_SNPRINTF(&buf[sz], bufsz - sz, "%lld", static_cast<long long>(reflect->GetInt64(msg, fds[i])));
                    if (res > 0) {
                        sz += static_cast<size_t>(res);
                    }
                    break;
                }
                case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:{
                    int res = UTIL_STRFUNC_SNPRINTF(&buf[sz], bufsz - sz, "%u", static_cast<unsigned int>(reflect->GetUInt32(msg, fds[i])));
                    if (res > 0) {
                        sz += static_cast<size_t>(res);
                    }
                    break;
                }
                case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:{
                    int res = UTIL_STRFUNC_SNPRINTF(&buf[sz], bufsz - sz, "%llu", static_cast<unsigned long long>(reflect->GetUInt64(msg, fds[i])));
                    if (res > 0) {
                        sz += static_cast<size_t>(res);
                    }
                    break;
                }
                case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:{
                    int res = UTIL_STRFUNC_SNPRINTF(&buf[sz], bufsz - sz, "%lg", static_cast<double>(reflect->GetDouble(msg, fds[i])));
                    if (res > 0) {
                        sz += static_cast<size_t>(res);
                    }
                    break;
                }
                case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:{
                    int res = UTIL_STRFUNC_SNPRINTF(&buf[sz], bufsz - sz, "%g", static_cast<float>(reflect->GetFloat(msg, fds[i])));
                    if (res > 0) {
                        sz += static_cast<size_t>(res);
                    }
                    break;
                }
                case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:{
                    int res;
                    if (reflect->GetBool(msg, fds[i])){
                        res = UTIL_STRFUNC_SNPRINTF(&buf[sz], bufsz - sz, "%s", "true");
                    } else {
                        res = UTIL_STRFUNC_SNPRINTF(&buf[sz], bufsz - sz, "%s", "false");
                    }
                    if (res > 0) {
                        sz += static_cast<size_t>(res);
                    }
                    break;
                }
                case google::protobuf::FieldDescriptor::CPPTYPE_ENUM: {
                    const ::google::protobuf::EnumValueDescriptor* val = reflect->GetEnum(msg, fds[i]);
                    int res = UTIL_STRFUNC_SNPRINTF(&buf[sz], bufsz - sz, "%s", val->name().c_str());
                    if (res > 0) {
                        sz += static_cast<size_t>(res);
                    }
                    break;
                }

                case google::protobuf::FieldDescriptor::CPPTYPE_STRING: {
                    std::string default_val;
                    const std::string& val = reflect->GetStringReference(msg, fds[i], &default_val);
                    int res = 0;
                    if (google::protobuf::FieldDescriptor::TYPE_BYTES != fds[i]->type()) {
                        res = UTIL_STRFUNC_SNPRINTF(&buf[sz], bufsz - sz, "%s", val.c_str());
                    } else {
                        res = UTIL_STRFUNC_SNPRINTF(&buf[sz], bufsz - sz, "BYTES(%llu)", static_cast<unsigned long long>(val.size()));
                    }

                    if (res > 0) {
                        sz += static_cast<size_t>(res);
                    }
                    break;
                }

                case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:{
                    sz += protobuf_mini_dumper_dump_readable(reflect->GetMessage(msg, fds[i]), &buf[sz], bufsz - sz, ident);
                    break;
                }
            }
        }

        // 尾部逗号
        if (sz < bufsz && i != fds.size() - 1) {
            buf[sz ++] = ',';
        }
    }

    if (short_mode) {
        if (sz < bufsz) {
            buf[sz++] = ' ';
        }
    } else {
        if (sz < bufsz) {
            buf[sz++] = '\n';
        }

        // 缩进
        for (int j = 1; j < ident; ++ j) {
            if (sz < bufsz - 1) {
                buf[sz ++] = ' ';
                buf[sz ++] = ' ';
            } else {
                break;
            }
        }
    }

    if (sz < bufsz) {
        buf[sz++] = '}';
    }

    if (sz < bufsz) {
        buf[sz] = 0;
    }
    return sz;
}