/*
 * session.h -- drmHandshake session activation protocol.
 *
 * Wraps libimobiledevice's session-mode mobileactivation API for the
 * drmHandshake protocol required since iOS 10.2+.
 *
 * Flow:
 *   1. session_get_info()                -> device session blob
 *   2. session_drm_handshake()           -> local handshake response
 *   3. session_create_activation_info()  -> activation info with session
 *   4. session_activate()                -> finalize activation
 */

#ifndef SESSION_H
#define SESSION_H

#include <plist/plist.h>
#include "device/device.h"

/*
 * session_get_info -- Retrieve session info blob from device.
 * Sends CreateTunnel1SessionInfoRequest via mobileactivation service.
 * On success, *session_info is set to a new plist that the caller must
 * free with plist_free().
 * Returns 0 on success, -1 on error.
 */
int session_get_info(device_info_t *dev, plist_t *session_info);

/*
 * session_drm_handshake -- Perform local drmHandshake using session info.
 * In offline mode, constructs the handshake response locally from the
 * session blob (CollectionBlob, HandshakeRequestMessage, UniqueDeviceID).
 * On success, *handshake_response is set to a new plist dict containing
 * FDRBlob, SUInfo, HandshakeResponseMessage, and serverKP.
 * Caller must free with plist_free().
 * Returns 0 on success, -1 on error.
 */
int session_drm_handshake(device_info_t *dev, plist_t session_info,
                          plist_t *handshake_response);

/*
 * session_create_activation_info -- Create activation info with session.
 * Sends CreateTunnel1ActivationInfoRequest using the handshake response
 * to produce the activation info blob.
 * On success, *activation_info is set to a new plist. Caller frees.
 * Returns 0 on success, -1 on error.
 */
int session_create_activation_info(device_info_t *dev,
                                   plist_t handshake_response,
                                   plist_t *activation_info);

/*
 * session_activate -- Finalize session-mode activation.
 * Sends HandleActivationInfoWithSessionRequest with the activation record
 * and handshake response. This completes the drmHandshake protocol.
 * Returns 0 on success, -1 on error.
 */
int session_activate(device_info_t *dev, plist_t activation_record,
                     plist_t handshake_response);

#endif /* SESSION_H */
