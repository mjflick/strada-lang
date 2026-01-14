/*
 * strada_usb.c - libusb wrapper for Strada FFI
 *
 * This library provides USB device access from Strada via libusb-1.0.
 * All functions accept StradaValue* arguments directly from Strada's FFI.
 *
 * Compile with:
 *   gcc -shared -fPIC -o libstrada_usb.so strada_usb.c -lusb-1.0 -I../../runtime
 *
 * Requirements:
 *   Ubuntu/Debian: apt install libusb-1.0-0-dev
 *   RHEL/CentOS:   yum install libusb1-devel
 *   macOS:         brew install libusb
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libusb-1.0/libusb.h>

/* Include Strada runtime for StradaValue type */
#include "strada_runtime.h"

/* Global libusb context */
static libusb_context *usb_ctx = NULL;
static int usb_initialized = 0;

/* ===== Initialization ===== */

/* Initialize libusb
 * Returns 0 on success, negative error code on failure */
int strada_usb_init(void) {
    if (usb_initialized) {
        return 0;
    }
    int ret = libusb_init(&usb_ctx);
    if (ret == 0) {
        usb_initialized = 1;
    }
    return ret;
}

/* Cleanup libusb */
void strada_usb_exit(void) {
    if (usb_initialized && usb_ctx) {
        libusb_exit(usb_ctx);
        usb_ctx = NULL;
        usb_initialized = 0;
    }
}

/* Set debug level (0=none, 1=error, 2=warning, 3=info, 4=debug) */
void strada_usb_set_debug(StradaValue *level_sv) {
    if (!usb_initialized) strada_usb_init();
    int level = (int)strada_to_int(level_sv);
#if LIBUSB_API_VERSION >= 0x01000106
    libusb_set_option(usb_ctx, LIBUSB_OPTION_LOG_LEVEL, level);
#else
    libusb_set_debug(usb_ctx, level);
#endif
}

/* Get libusb error string */
StradaValue* strada_usb_strerror(StradaValue *errcode_sv) {
    int errcode = (int)strada_to_int(errcode_sv);
    const char *str = libusb_strerror(errcode);
    return strada_new_str(str ? str : "Unknown error");
}

/* ===== Device Enumeration ===== */

/* Get list of USB devices
 * Returns array of device info hashes with vid, pid, bus, address */
StradaValue* strada_usb_get_device_list(void) {
    if (!usb_initialized) strada_usb_init();

    libusb_device **list;
    ssize_t count = libusb_get_device_list(usb_ctx, &list);

    StradaValue *result = strada_new_array();

    if (count < 0) {
        return result;
    }

    for (ssize_t i = 0; i < count; i++) {
        struct libusb_device_descriptor desc;
        if (libusb_get_device_descriptor(list[i], &desc) == 0) {
            StradaValue *dev_info = strada_new_hash();

            strada_hash_set(dev_info->value.hv, "vid",
                strada_new_int(desc.idVendor));
            strada_hash_set(dev_info->value.hv, "pid",
                strada_new_int(desc.idProduct));
            strada_hash_set(dev_info->value.hv, "bus",
                strada_new_int(libusb_get_bus_number(list[i])));
            strada_hash_set(dev_info->value.hv, "address",
                strada_new_int(libusb_get_device_address(list[i])));
            strada_hash_set(dev_info->value.hv, "class",
                strada_new_int(desc.bDeviceClass));
            strada_hash_set(dev_info->value.hv, "subclass",
                strada_new_int(desc.bDeviceSubClass));
            strada_hash_set(dev_info->value.hv, "protocol",
                strada_new_int(desc.bDeviceProtocol));

            /* Format VID:PID string */
            char vidpid[16];
            snprintf(vidpid, sizeof(vidpid), "%04x:%04x",
                desc.idVendor, desc.idProduct);
            strada_hash_set(dev_info->value.hv, "vidpid",
                strada_new_str(vidpid));

            strada_array_push(result->value.av, dev_info);
        }
    }

    libusb_free_device_list(list, 1);
    return result;
}

/* ===== Device Open/Close ===== */

/* Open device by VID:PID
 * Returns device handle (as int) or 0 on failure */
StradaValue* strada_usb_open_device(StradaValue *vid_sv, StradaValue *pid_sv) {
    if (!usb_initialized) strada_usb_init();

    uint16_t vid = (uint16_t)strada_to_int(vid_sv);
    uint16_t pid = (uint16_t)strada_to_int(pid_sv);

    libusb_device_handle *handle = libusb_open_device_with_vid_pid(usb_ctx, vid, pid);
    if (!handle) {
        return strada_new_int(0);
    }

    return strada_new_int((int64_t)(intptr_t)handle);
}

/* Open device by bus and address
 * Returns device handle or 0 on failure */
StradaValue* strada_usb_open_device_by_path(StradaValue *bus_sv, StradaValue *addr_sv) {
    if (!usb_initialized) strada_usb_init();

    int target_bus = (int)strada_to_int(bus_sv);
    int target_addr = (int)strada_to_int(addr_sv);

    libusb_device **list;
    ssize_t count = libusb_get_device_list(usb_ctx, &list);
    if (count < 0) {
        return strada_new_int(0);
    }

    libusb_device_handle *handle = NULL;

    for (ssize_t i = 0; i < count; i++) {
        int bus = libusb_get_bus_number(list[i]);
        int addr = libusb_get_device_address(list[i]);

        if (bus == target_bus && addr == target_addr) {
            if (libusb_open(list[i], &handle) != 0) {
                handle = NULL;
            }
            break;
        }
    }

    libusb_free_device_list(list, 1);
    return strada_new_int((int64_t)(intptr_t)handle);
}

/* Close device handle */
void strada_usb_close(StradaValue *handle_sv) {
    libusb_device_handle *handle = (libusb_device_handle *)(intptr_t)strada_to_int(handle_sv);
    if (handle) {
        libusb_close(handle);
    }
}

/* ===== Interface Management ===== */

/* Claim interface for exclusive use
 * Returns 0 on success, negative error code on failure */
StradaValue* strada_usb_claim_interface(StradaValue *handle_sv, StradaValue *iface_sv) {
    libusb_device_handle *handle = (libusb_device_handle *)(intptr_t)strada_to_int(handle_sv);
    int iface = (int)strada_to_int(iface_sv);

    if (!handle) {
        return strada_new_int(LIBUSB_ERROR_INVALID_PARAM);
    }

    return strada_new_int(libusb_claim_interface(handle, iface));
}

/* Release interface */
StradaValue* strada_usb_release_interface(StradaValue *handle_sv, StradaValue *iface_sv) {
    libusb_device_handle *handle = (libusb_device_handle *)(intptr_t)strada_to_int(handle_sv);
    int iface = (int)strada_to_int(iface_sv);

    if (!handle) {
        return strada_new_int(LIBUSB_ERROR_INVALID_PARAM);
    }

    return strada_new_int(libusb_release_interface(handle, iface));
}

/* Detach kernel driver from interface (Linux only) */
StradaValue* strada_usb_detach_kernel_driver(StradaValue *handle_sv, StradaValue *iface_sv) {
    libusb_device_handle *handle = (libusb_device_handle *)(intptr_t)strada_to_int(handle_sv);
    int iface = (int)strada_to_int(iface_sv);

    if (!handle) {
        return strada_new_int(LIBUSB_ERROR_INVALID_PARAM);
    }

    return strada_new_int(libusb_detach_kernel_driver(handle, iface));
}

/* Check if kernel driver is active on interface */
StradaValue* strada_usb_kernel_driver_active(StradaValue *handle_sv, StradaValue *iface_sv) {
    libusb_device_handle *handle = (libusb_device_handle *)(intptr_t)strada_to_int(handle_sv);
    int iface = (int)strada_to_int(iface_sv);

    if (!handle) {
        return strada_new_int(LIBUSB_ERROR_INVALID_PARAM);
    }

    return strada_new_int(libusb_kernel_driver_active(handle, iface));
}

/* Set auto-detach kernel driver mode */
StradaValue* strada_usb_set_auto_detach(StradaValue *handle_sv, StradaValue *enable_sv) {
    libusb_device_handle *handle = (libusb_device_handle *)(intptr_t)strada_to_int(handle_sv);
    int enable = (int)strada_to_int(enable_sv);

    if (!handle) {
        return strada_new_int(LIBUSB_ERROR_INVALID_PARAM);
    }

    return strada_new_int(libusb_set_auto_detach_kernel_driver(handle, enable));
}

/* ===== Data Transfers ===== */

/* Bulk transfer (read or write based on endpoint direction)
 * endpoint: 0x01-0x0F for OUT, 0x81-0x8F for IN
 * Returns: data string on read, bytes transferred count on write */
StradaValue* strada_usb_bulk_transfer(StradaValue *handle_sv, StradaValue *endpoint_sv,
                                       StradaValue *data_sv, StradaValue *timeout_sv) {
    libusb_device_handle *handle = (libusb_device_handle *)(intptr_t)strada_to_int(handle_sv);
    unsigned char endpoint = (unsigned char)strada_to_int(endpoint_sv);
    int timeout = (int)strada_to_int(timeout_sv);

    if (!handle) {
        return strada_new_int(LIBUSB_ERROR_INVALID_PARAM);
    }

    int transferred = 0;

    /* Check endpoint direction */
    if (endpoint & 0x80) {
        /* IN endpoint - read data */
        int length = (int)strada_to_int(data_sv);  /* data_sv is the length for reads */
        unsigned char *buffer = malloc(length);
        if (!buffer) {
            return strada_new_str("");
        }

        int ret = libusb_bulk_transfer(handle, endpoint, buffer, length, &transferred, timeout);
        if (ret < 0 && ret != LIBUSB_ERROR_TIMEOUT) {
            free(buffer);
            return strada_new_str("");
        }

        StradaValue *result = strada_new_str_len((char *)buffer, transferred);
        free(buffer);
        return result;
    } else {
        /* OUT endpoint - write data */
        const char *data = strada_to_str(data_sv);
        int length = data_sv->struct_size > 0 ? (int)data_sv->struct_size : (int)strlen(data);

        int ret = libusb_bulk_transfer(handle, endpoint, (unsigned char *)data,
                                        length, &transferred, timeout);
        if (ret < 0) {
            return strada_new_int(ret);
        }
        return strada_new_int(transferred);
    }
}

/* Interrupt transfer */
StradaValue* strada_usb_interrupt_transfer(StradaValue *handle_sv, StradaValue *endpoint_sv,
                                            StradaValue *data_sv, StradaValue *timeout_sv) {
    libusb_device_handle *handle = (libusb_device_handle *)(intptr_t)strada_to_int(handle_sv);
    unsigned char endpoint = (unsigned char)strada_to_int(endpoint_sv);
    int timeout = (int)strada_to_int(timeout_sv);

    if (!handle) {
        return strada_new_int(LIBUSB_ERROR_INVALID_PARAM);
    }

    int transferred = 0;

    if (endpoint & 0x80) {
        /* IN endpoint - read */
        int length = (int)strada_to_int(data_sv);
        unsigned char *buffer = malloc(length);
        if (!buffer) {
            return strada_new_str("");
        }

        int ret = libusb_interrupt_transfer(handle, endpoint, buffer, length, &transferred, timeout);
        if (ret < 0 && ret != LIBUSB_ERROR_TIMEOUT) {
            free(buffer);
            return strada_new_str("");
        }

        StradaValue *result = strada_new_str_len((char *)buffer, transferred);
        free(buffer);
        return result;
    } else {
        /* OUT endpoint - write */
        const char *data = strada_to_str(data_sv);
        int length = data_sv->struct_size > 0 ? (int)data_sv->struct_size : (int)strlen(data);

        int ret = libusb_interrupt_transfer(handle, endpoint, (unsigned char *)data,
                                             length, &transferred, timeout);
        if (ret < 0) {
            return strada_new_int(ret);
        }
        return strada_new_int(transferred);
    }
}

/* Control transfer
 * For vendor/class specific requests */
StradaValue* strada_usb_control_transfer(StradaValue *handle_sv,
                                          StradaValue *request_type_sv,
                                          StradaValue *request_sv,
                                          StradaValue *value_sv,
                                          StradaValue *index_sv,
                                          StradaValue *data_sv,
                                          StradaValue *timeout_sv) {
    libusb_device_handle *handle = (libusb_device_handle *)(intptr_t)strada_to_int(handle_sv);
    uint8_t request_type = (uint8_t)strada_to_int(request_type_sv);
    uint8_t request = (uint8_t)strada_to_int(request_sv);
    uint16_t value = (uint16_t)strada_to_int(value_sv);
    uint16_t index = (uint16_t)strada_to_int(index_sv);
    unsigned int timeout = (unsigned int)strada_to_int(timeout_sv);

    if (!handle) {
        return strada_new_int(LIBUSB_ERROR_INVALID_PARAM);
    }

    /* Check if this is a read or write based on request_type bit 7 */
    if (request_type & 0x80) {
        /* Device-to-host (read) */
        int length = (int)strada_to_int(data_sv);
        unsigned char *buffer = malloc(length);
        if (!buffer) {
            return strada_new_str("");
        }

        int ret = libusb_control_transfer(handle, request_type, request,
                                           value, index, buffer, length, timeout);
        if (ret < 0) {
            free(buffer);
            return strada_new_str("");
        }

        StradaValue *result = strada_new_str_len((char *)buffer, ret);
        free(buffer);
        return result;
    } else {
        /* Host-to-device (write) */
        const char *data = strada_to_str(data_sv);
        int length = data_sv->struct_size > 0 ? (int)data_sv->struct_size : (int)strlen(data);

        int ret = libusb_control_transfer(handle, request_type, request,
                                           value, index, (unsigned char *)data, length, timeout);
        return strada_new_int(ret);
    }
}

/* ===== Device Descriptors ===== */

/* Get device descriptor
 * Returns hash with device info */
StradaValue* strada_usb_get_device_descriptor(StradaValue *handle_sv) {
    libusb_device_handle *handle = (libusb_device_handle *)(intptr_t)strada_to_int(handle_sv);
    if (!handle) {
        return strada_new_undef();
    }

    libusb_device *dev = libusb_get_device(handle);
    struct libusb_device_descriptor desc;

    if (libusb_get_device_descriptor(dev, &desc) != 0) {
        return strada_new_undef();
    }

    StradaValue *result = strada_new_hash();
    strada_hash_set(result->value.hv, "usb_version",
        strada_new_int(desc.bcdUSB));
    strada_hash_set(result->value.hv, "device_class",
        strada_new_int(desc.bDeviceClass));
    strada_hash_set(result->value.hv, "device_subclass",
        strada_new_int(desc.bDeviceSubClass));
    strada_hash_set(result->value.hv, "device_protocol",
        strada_new_int(desc.bDeviceProtocol));
    strada_hash_set(result->value.hv, "max_packet_size",
        strada_new_int(desc.bMaxPacketSize0));
    strada_hash_set(result->value.hv, "vendor_id",
        strada_new_int(desc.idVendor));
    strada_hash_set(result->value.hv, "product_id",
        strada_new_int(desc.idProduct));
    strada_hash_set(result->value.hv, "device_version",
        strada_new_int(desc.bcdDevice));
    strada_hash_set(result->value.hv, "num_configurations",
        strada_new_int(desc.bNumConfigurations));
    strada_hash_set(result->value.hv, "manufacturer_index",
        strada_new_int(desc.iManufacturer));
    strada_hash_set(result->value.hv, "product_index",
        strada_new_int(desc.iProduct));
    strada_hash_set(result->value.hv, "serial_index",
        strada_new_int(desc.iSerialNumber));

    return result;
}

/* Get string descriptor */
StradaValue* strada_usb_get_string_descriptor(StradaValue *handle_sv, StradaValue *index_sv) {
    libusb_device_handle *handle = (libusb_device_handle *)(intptr_t)strada_to_int(handle_sv);
    int index = (int)strada_to_int(index_sv);

    if (!handle || index == 0) {
        return strada_new_str("");
    }

    unsigned char buffer[256];
    int ret = libusb_get_string_descriptor_ascii(handle, index, buffer, sizeof(buffer));

    if (ret < 0) {
        return strada_new_str("");
    }

    return strada_new_str((char *)buffer);
}

/* Get configuration descriptor
 * Returns hash with configuration info */
StradaValue* strada_usb_get_config_descriptor(StradaValue *handle_sv, StradaValue *config_sv) {
    libusb_device_handle *handle = (libusb_device_handle *)(intptr_t)strada_to_int(handle_sv);
    int config_index = (int)strada_to_int(config_sv);

    if (!handle) {
        return strada_new_undef();
    }

    libusb_device *dev = libusb_get_device(handle);
    struct libusb_config_descriptor *config;

    if (libusb_get_config_descriptor(dev, config_index, &config) != 0) {
        return strada_new_undef();
    }

    StradaValue *result = strada_new_hash();
    strada_hash_set(result->value.hv, "num_interfaces",
        strada_new_int(config->bNumInterfaces));
    strada_hash_set(result->value.hv, "configuration_value",
        strada_new_int(config->bConfigurationValue));
    strada_hash_set(result->value.hv, "max_power",
        strada_new_int(config->MaxPower * 2));  /* In mA */
    strada_hash_set(result->value.hv, "attributes",
        strada_new_int(config->bmAttributes));

    /* Build array of interfaces */
    StradaValue *interfaces = strada_new_array();
    for (int i = 0; i < config->bNumInterfaces; i++) {
        const struct libusb_interface *iface = &config->interface[i];
        for (int j = 0; j < iface->num_altsetting; j++) {
            const struct libusb_interface_descriptor *alt = &iface->altsetting[j];

            StradaValue *iface_info = strada_new_hash();
            strada_hash_set(iface_info->value.hv, "interface_number",
                strada_new_int(alt->bInterfaceNumber));
            strada_hash_set(iface_info->value.hv, "alt_setting",
                strada_new_int(alt->bAlternateSetting));
            strada_hash_set(iface_info->value.hv, "interface_class",
                strada_new_int(alt->bInterfaceClass));
            strada_hash_set(iface_info->value.hv, "interface_subclass",
                strada_new_int(alt->bInterfaceSubClass));
            strada_hash_set(iface_info->value.hv, "interface_protocol",
                strada_new_int(alt->bInterfaceProtocol));
            strada_hash_set(iface_info->value.hv, "num_endpoints",
                strada_new_int(alt->bNumEndpoints));

            /* Build array of endpoints */
            StradaValue *endpoints = strada_new_array();
            for (int k = 0; k < alt->bNumEndpoints; k++) {
                const struct libusb_endpoint_descriptor *ep = &alt->endpoint[k];

                StradaValue *ep_info = strada_new_hash();
                strada_hash_set(ep_info->value.hv, "address",
                    strada_new_int(ep->bEndpointAddress));
                strada_hash_set(ep_info->value.hv, "attributes",
                    strada_new_int(ep->bmAttributes));
                strada_hash_set(ep_info->value.hv, "max_packet_size",
                    strada_new_int(ep->wMaxPacketSize));
                strada_hash_set(ep_info->value.hv, "interval",
                    strada_new_int(ep->bInterval));

                /* Decode transfer type */
                const char *type_str = "unknown";
                switch (ep->bmAttributes & 0x03) {
                    case LIBUSB_TRANSFER_TYPE_CONTROL: type_str = "control"; break;
                    case LIBUSB_TRANSFER_TYPE_ISOCHRONOUS: type_str = "isochronous"; break;
                    case LIBUSB_TRANSFER_TYPE_BULK: type_str = "bulk"; break;
                    case LIBUSB_TRANSFER_TYPE_INTERRUPT: type_str = "interrupt"; break;
                }
                strada_hash_set(ep_info->value.hv, "type", strada_new_str(type_str));

                /* Decode direction */
                strada_hash_set(ep_info->value.hv, "direction",
                    strada_new_str((ep->bEndpointAddress & 0x80) ? "in" : "out"));

                strada_array_push(endpoints->value.av, ep_info);
            }
            strada_hash_set(iface_info->value.hv, "endpoints", endpoints);

            strada_array_push(interfaces->value.av, iface_info);
        }
    }
    strada_hash_set(result->value.hv, "interfaces", interfaces);

    libusb_free_config_descriptor(config);
    return result;
}

/* ===== Configuration ===== */

/* Get active configuration */
StradaValue* strada_usb_get_configuration(StradaValue *handle_sv) {
    libusb_device_handle *handle = (libusb_device_handle *)(intptr_t)strada_to_int(handle_sv);
    if (!handle) {
        return strada_new_int(LIBUSB_ERROR_INVALID_PARAM);
    }

    int config;
    int ret = libusb_get_configuration(handle, &config);
    if (ret < 0) {
        return strada_new_int(ret);
    }
    return strada_new_int(config);
}

/* Set configuration */
StradaValue* strada_usb_set_configuration(StradaValue *handle_sv, StradaValue *config_sv) {
    libusb_device_handle *handle = (libusb_device_handle *)(intptr_t)strada_to_int(handle_sv);
    int config = (int)strada_to_int(config_sv);

    if (!handle) {
        return strada_new_int(LIBUSB_ERROR_INVALID_PARAM);
    }

    return strada_new_int(libusb_set_configuration(handle, config));
}

/* Set alternate setting for interface */
StradaValue* strada_usb_set_interface_alt_setting(StradaValue *handle_sv,
                                                    StradaValue *iface_sv,
                                                    StradaValue *alt_sv) {
    libusb_device_handle *handle = (libusb_device_handle *)(intptr_t)strada_to_int(handle_sv);
    int iface = (int)strada_to_int(iface_sv);
    int alt = (int)strada_to_int(alt_sv);

    if (!handle) {
        return strada_new_int(LIBUSB_ERROR_INVALID_PARAM);
    }

    return strada_new_int(libusb_set_interface_alt_setting(handle, iface, alt));
}

/* Clear halt/stall on endpoint */
StradaValue* strada_usb_clear_halt(StradaValue *handle_sv, StradaValue *endpoint_sv) {
    libusb_device_handle *handle = (libusb_device_handle *)(intptr_t)strada_to_int(handle_sv);
    unsigned char endpoint = (unsigned char)strada_to_int(endpoint_sv);

    if (!handle) {
        return strada_new_int(LIBUSB_ERROR_INVALID_PARAM);
    }

    return strada_new_int(libusb_clear_halt(handle, endpoint));
}

/* Reset device */
StradaValue* strada_usb_reset_device(StradaValue *handle_sv) {
    libusb_device_handle *handle = (libusb_device_handle *)(intptr_t)strada_to_int(handle_sv);
    if (!handle) {
        return strada_new_int(LIBUSB_ERROR_INVALID_PARAM);
    }

    return strada_new_int(libusb_reset_device(handle));
}

/* ===== USB Class Constants ===== */

/* Return USB class code constants */
StradaValue* strada_usb_class_hid(void) { return strada_new_int(LIBUSB_CLASS_HID); }
StradaValue* strada_usb_class_mass_storage(void) { return strada_new_int(LIBUSB_CLASS_MASS_STORAGE); }
StradaValue* strada_usb_class_hub(void) { return strada_new_int(LIBUSB_CLASS_HUB); }
StradaValue* strada_usb_class_vendor_spec(void) { return strada_new_int(LIBUSB_CLASS_VENDOR_SPEC); }
StradaValue* strada_usb_class_printer(void) { return strada_new_int(LIBUSB_CLASS_PRINTER); }
StradaValue* strada_usb_class_audio(void) { return strada_new_int(LIBUSB_CLASS_AUDIO); }
StradaValue* strada_usb_class_video(void) { return strada_new_int(LIBUSB_CLASS_VIDEO); }
StradaValue* strada_usb_class_comm(void) { return strada_new_int(LIBUSB_CLASS_COMM); }

/* Request type constants for control transfers */
StradaValue* strada_usb_request_type_standard(void) { return strada_new_int(LIBUSB_REQUEST_TYPE_STANDARD); }
StradaValue* strada_usb_request_type_class(void) { return strada_new_int(LIBUSB_REQUEST_TYPE_CLASS); }
StradaValue* strada_usb_request_type_vendor(void) { return strada_new_int(LIBUSB_REQUEST_TYPE_VENDOR); }
StradaValue* strada_usb_recipient_device(void) { return strada_new_int(LIBUSB_RECIPIENT_DEVICE); }
StradaValue* strada_usb_recipient_interface(void) { return strada_new_int(LIBUSB_RECIPIENT_INTERFACE); }
StradaValue* strada_usb_recipient_endpoint(void) { return strada_new_int(LIBUSB_RECIPIENT_ENDPOINT); }
StradaValue* strada_usb_endpoint_in(void) { return strada_new_int(LIBUSB_ENDPOINT_IN); }
StradaValue* strada_usb_endpoint_out(void) { return strada_new_int(LIBUSB_ENDPOINT_OUT); }

/* Standard request codes */
StradaValue* strada_usb_request_get_status(void) { return strada_new_int(LIBUSB_REQUEST_GET_STATUS); }
StradaValue* strada_usb_request_clear_feature(void) { return strada_new_int(LIBUSB_REQUEST_CLEAR_FEATURE); }
StradaValue* strada_usb_request_set_feature(void) { return strada_new_int(LIBUSB_REQUEST_SET_FEATURE); }
StradaValue* strada_usb_request_get_descriptor(void) { return strada_new_int(LIBUSB_REQUEST_GET_DESCRIPTOR); }
StradaValue* strada_usb_request_set_descriptor(void) { return strada_new_int(LIBUSB_REQUEST_SET_DESCRIPTOR); }
StradaValue* strada_usb_request_get_configuration(void) { return strada_new_int(LIBUSB_REQUEST_GET_CONFIGURATION); }
StradaValue* strada_usb_request_set_configuration(void) { return strada_new_int(LIBUSB_REQUEST_SET_CONFIGURATION); }

/* Error code constants */
StradaValue* strada_usb_error_io(void) { return strada_new_int(LIBUSB_ERROR_IO); }
StradaValue* strada_usb_error_invalid_param(void) { return strada_new_int(LIBUSB_ERROR_INVALID_PARAM); }
StradaValue* strada_usb_error_access(void) { return strada_new_int(LIBUSB_ERROR_ACCESS); }
StradaValue* strada_usb_error_no_device(void) { return strada_new_int(LIBUSB_ERROR_NO_DEVICE); }
StradaValue* strada_usb_error_not_found(void) { return strada_new_int(LIBUSB_ERROR_NOT_FOUND); }
StradaValue* strada_usb_error_busy(void) { return strada_new_int(LIBUSB_ERROR_BUSY); }
StradaValue* strada_usb_error_timeout(void) { return strada_new_int(LIBUSB_ERROR_TIMEOUT); }
StradaValue* strada_usb_error_overflow(void) { return strada_new_int(LIBUSB_ERROR_OVERFLOW); }
StradaValue* strada_usb_error_pipe(void) { return strada_new_int(LIBUSB_ERROR_PIPE); }
StradaValue* strada_usb_error_interrupted(void) { return strada_new_int(LIBUSB_ERROR_INTERRUPTED); }
StradaValue* strada_usb_error_no_mem(void) { return strada_new_int(LIBUSB_ERROR_NO_MEM); }
StradaValue* strada_usb_error_not_supported(void) { return strada_new_int(LIBUSB_ERROR_NOT_SUPPORTED); }
