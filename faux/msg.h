/** @file msg.h
 *
 * @brief Library to implement simple network protocol
 *
 * Message (request or answer) has the following structure:
 *
 * [ Message header ]
 * [ First parameter's header ]
 * [ ... ]
 * [ N-th parameter's header ]
 * [ First parameter's data ]
 * [ ... ]
 * [ N-th parameter's data ]
 *
 * Message header has such standard fields as:
 * - Magic number
 * - Protocol version major number
 * - Protocol version minor number
 * - Command code
 * - Status
 * - Request ID field to identify session
 * - Number of parameters
 * - Whole message length
 *
 * The length of message can vary. Header has constant length and
 * contains total length of whole message and number of parameters. There are
 * parameter headers for each parameter. Parameter's header contains parameter
 * type and length of parameter. After parameter headers array the parameter's
 * data follows in a sequence.
 */


#ifndef _faux_msg_h
#define _faux_msg_h

#include <stdint.h>
#include <signal.h>
#include <faux/faux.h>
#include <faux/list.h>
#include <faux/net.h>

typedef struct faux_msg_s faux_msg_t;

// Debug variable. BOOL_TRUE for debug and BOOL_FALSE to switch debug off
extern bool_t faux_msg_debug_flag;


/** @brief Parameter header
 */
typedef struct faux_phdr_s {
	uint16_t param_type; // Parameter type
	uint8_t reserved[2];
	uint32_t param_len; // Length of parameter (not including header)
} faux_phdr_t;


/** @brief Message header
 */
typedef struct faux_hdr_s {
	uint32_t magic; // Magic number
	uint8_t major; // Major protocol version number
	uint8_t minor; // Minor protocol version number
	uint16_t cmd; // Command
	uint32_t status; // Status
	uint32_t req_id; // Some abstract ID of request. Identifies request/answer
	uint32_t param_num; // Number of parameters
	uint32_t len; // Length of whole message (including header)
	faux_phdr_t phdr[]; // Parameter headers (unknown length)
} faux_hdr_t;


C_DECL_BEGIN

// Parameter functions
void faux_phdr_set_type(faux_phdr_t *phdr, uint16_t param_type);
uint16_t faux_phdr_get_type(const faux_phdr_t *phdr);
void faux_phdr_set_len(faux_phdr_t *phdr, uint32_t param_len);
uint32_t faux_phdr_get_len(const faux_phdr_t *phdr);

// Message functions
faux_msg_t *faux_msg_new(uint32_t magic, uint8_t major, uint8_t minor);
void faux_msg_free(faux_msg_t *msg);
void faux_msg_set_cmd(faux_msg_t *msg, uint16_t cmd);
uint16_t faux_msg_get_cmd(const faux_msg_t *msg);
void faux_msg_set_status(faux_msg_t *msg, uint32_t status);
uint32_t faux_msg_get_status(const faux_msg_t *msg);
void faux_msg_set_req_id(faux_msg_t *msg, uint32_t req_id);
uint32_t faux_msg_get_req_id(const faux_msg_t *msg);
uint32_t faux_msg_get_param_num(const faux_msg_t *msg);
int faux_msg_get_len(const faux_msg_t *msg);
uint32_t faux_msg_get_magic(const faux_msg_t *msg);
int faux_msg_get_major(const faux_msg_t *msg);
int faux_msg_get_minor(const faux_msg_t *msg);
ssize_t faux_msg_add_param(faux_msg_t *msg, uint16_t type,
	const void *buf, size_t len);
faux_list_node_t *faux_msg_init_param_iter(const faux_msg_t *msg);
faux_phdr_t *faux_msg_get_param_each(faux_list_node_t **node,
	uint16_t *param_type, void **param_data, uint32_t *param_len);
faux_phdr_t *faux_msg_get_param_by_index(const faux_msg_t *msg, unsigned int index,
	uint16_t *param_type, void **param_data, uint32_t *param_len);
faux_phdr_t *faux_msg_get_param_by_type(const faux_msg_t *msg,
	uint16_t param_type, void **param_data, uint32_t *param_len);
ssize_t faux_msg_send(faux_msg_t *msg, faux_net_t *faux_net);
faux_msg_t *faux_msg_recv(faux_net_t *faux_net);
void faux_msg_debug(faux_msg_t *msg);

C_DECL_END

#endif // _faux_msg_h
