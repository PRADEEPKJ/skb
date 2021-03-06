/*
 * Copyright (c) 2007, 2008, 2009, ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Haldeneggsteig 4, CH-8092 Zurich. Attn: Systems Group.
 */

#ifndef SKB_H_
#define SKB_H_

#include <stdint.h>    /* for int32_t */
#include <sys/cdefs.h> /* for __BEGIN_DECLS, __END_DECLS */
#include <stdarg.h>
#include <stdbool.h>

typedef long errval_t;

#define BUFFER_SIZE (32 * 1024)
#define MEMORY_SIZE 32*1024*1024
#define ELEMENT_NAME_BUF_SIZE 80
#define ECLIPSE_DIR "/usr/local/eclipse"

struct skb_query_state {
    char output_buffer[BUFFER_SIZE];
    char error_buffer[BUFFER_SIZE];
    int output_length;
    int error_output_length;
    int exec_res;
};

__BEGIN_DECLS

errval_t skb_add_fact(char *fmt, ...) __attribute__((format(printf, 1, 2)));
errval_t skb_add_fact_my(char *fmt, ...) __attribute__((format(printf, 1, 2)));

struct list_parser_status {
    char *s;
    char *conv_ptr;
    size_t len;
    char element_name[ELEMENT_NAME_BUF_SIZE];
    int expected_conversions;
    int element_len;
};


/*skb interfaces from Barrelfish. Ones operating with output are not tested 
need to be reworked. */
int skb_init(void);
//int skb_read_error_code(void);
char *skb_get_output(void);
char *skb_get_error_output(void);
errval_t skb_execute(char *goal);
int skb_execute_query(char *fmt, ...) __attribute__((format(printf, 1, 2)));
errval_t skb_read_output_at(char *output, char *fmt, ...) __attribute__((format(scanf, 2, 3)));
errval_t skb_vread_output_at(char *output, char *fmt, va_list va_l);
errval_t skb_read_output(char *fmt, ...) __attribute__((format(scanf, 1, 2)));
void skb_read_list_init_offset(struct list_parser_status *status, char *s,
                               int offset);
void skb_read_list_init(struct list_parser_status *status);
bool skb_read_list(struct list_parser_status *status, char *fmt, ...)
    __attribute__((format(scanf, 2, 3)));
errval_t execute_query(char* query, struct skb_query_state* st);

__END_DECLS

#endif // SKB_H_
