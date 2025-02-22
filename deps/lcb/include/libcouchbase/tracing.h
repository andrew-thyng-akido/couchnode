/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2018-2020 Couchbase, Inc.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#ifndef LCB_TRACING_H
#define LCB_TRACING_H

/**
 * End to end tracing
 *
 * @ingroup lcb-public-api
 * @defgroup lcb-tracing-api End to end tracing
 * @brief Tracing operations through SDK and the Cluster.
 *
 * @addtogroup lcb-tracing-api
 * @{
 */

#ifdef __cplusplus
namespace lcb
{
namespace trace
{
class Span;
class Tracer;
} // namespace trace
} // namespace lcb
typedef lcb::trace::Span lcbtrace_SPAN;
extern "C" {
#else /* C only! */
typedef struct lcbtrace_SPAN_Cdummy lcbtrace_SPAN;
#endif

/**
 * Flag for @ref lcbtrace_new to request threshold logging tracer.
 */
#define LCBTRACE_F_THRESHOLD 0x01

/**
 * Flag for @ref lcbtrace_new to request external tracer.
 */
#define LCBTRACE_F_EXTERNAL 0x02

/**
 * Service the span is associated with.  Used in threshold logging tracer
 */
typedef enum {
    LCBTRACE_SERVICE_KV = 0,
    LCBTRACE_SERVICE_QUERY,
    LCBTRACE_SERVICE_VIEW,
    LCBTRACE_SERVICE_SEARCH,
    LCBTRACE_SERVICE_ANALYTICS,
    LCBTRACE_SERVICE__MAX
} lcbtrace_SERVICE;

struct lcbtrace_TRACER;
/**
 * Tracer interface.
 */
typedef struct lcbtrace_TRACER {
    lcb_U16 version;                                    /**< version of the structure, current value is 0 */
    lcb_U64 flags;                                      /**< tracer-specific flags */
    void *cookie;                                       /**< opaque pointer (e.g. pointer to wrapper structure) */
    void (*destructor)(struct lcbtrace_TRACER *tracer); /**< destructor function or NULL, if it is not necessary */
    union {
        struct {
            void (*report)(struct lcbtrace_TRACER *tracer, lcbtrace_SPAN *span); /**< optional reporter function */
        } v0;
        struct {
            void *(*start_span)(struct lcbtrace_TRACER *tracer, const char *name, void *parent);
            void (*end_span)(void *span);
            void (*destroy_span)(void *span);
            void (*add_tag_string)(void *span, const char *name, const char *value, size_t value_len);
            void (*add_tag_uint64)(void *span, const char *name, uint64_t value);
        } v1;
    } v;
} lcbtrace_TRACER;

/**
 * Get current tracer of the connection.
 *
 * @param instance current connection
 * @return tracer
 *
 * @committed
 */
LIBCOUCHBASE_API lcbtrace_TRACER *lcb_get_tracer(lcb_INSTANCE *instance);

/**
 * Set current tracer for the connection.
 *
 * @param instance current connection
 * @param tracer tracer instance
 *
 * @committed
 */
LIBCOUCHBASE_API void lcb_set_tracer(lcb_INSTANCE *instance, lcbtrace_TRACER *tracer);

/**
 * Create default libcouchbase tracer instance.
 *
 * @param instance current connection
 * @param flags pass @ref LCBTRACE_F_THRESHOLD if needed threshold logging tracer.
 * @return new tracer or NULL when nothing has been created.
 *
 * @committed
 */
LIBCOUCHBASE_API lcbtrace_TRACER *lcbtrace_new(lcb_INSTANCE *instance, lcb_U64 flags);

/**
 * Destroy tracer object.
 *
 * @committed
 */
LIBCOUCHBASE_API void lcbtrace_destroy(lcbtrace_TRACER *tracer);

typedef enum {
    LCBTRACE_REF_NONE = 0,
    LCBTRACE_REF_CHILD_OF = 1,
    LCBTRACE_REF_FOLLOWS_FROM,
    LCBTRACE_REF__MAX
} lcbtrace_REF_TYPE;

typedef struct {
    lcbtrace_REF_TYPE type;
    lcbtrace_SPAN *span;
} lcbtrace_REF;

/** zero means the library will trigger timestamp automatically */
#define LCBTRACE_NOW 0

/**
 * Get current timestamp.
 *
 * @return current wall clock time in microseconds
 *
 * @committed
 */
LIBCOUCHBASE_API
lcb_U64 lcbtrace_now(void);

/**
 * Start span.
 *
 * @param tracer tracer instance
 * @param operation the operation code
 * @param now start timestamp or @ref LCBTRACE_NOW to let the library to record current time from the wall clock.
 * @param ref reference to the other span, or NULL
 *
 * @committed
 */
LIBCOUCHBASE_API
lcbtrace_SPAN *lcbtrace_span_start(lcbtrace_TRACER *tracer, const char *operation, lcb_U64 now, lcbtrace_REF *ref);

/**
 * Wrap external span
 *
 * If using a custom tracer which has its own span representation, you can create an lcbtrace_SPAN that wraps
 * it.  This is useful when setting parent spans.
 *
 * @param tracer tracer instance.
 * @param operation operation code.
 * @param start timestamp from the external_span.
 * @param external_span pointer to the external span.
 * @param lcbspan  pointer to a new lcbtrace_SPAN* which wraps external_span.
 * @return LCB_SUCCESS when successful.
 *
 * @volatile
 */
LIBCOUCHBASE_API
lcb_STATUS lcbtrace_span_wrap(lcbtrace_TRACER *tracer, const char *opname, uint64_t start, void *external_span,
                              lcbtrace_SPAN **lcbspan);

/**
 * Mark the span as finished.
 *
 * @param span span instance
 * @param now finish timestamp or @ref LCBTRACE_NOW to let the library to record current time from the wall clock.
 *
 * @committed
 */
LIBCOUCHBASE_API
void lcbtrace_span_finish(lcbtrace_SPAN *span, lcb_U64 now);

/**
 * @private
 */
LIBCOUCHBASE_API
int lcbtrace_span_should_finish(lcbtrace_SPAN *span);

/**
 * Get start timestamp of the span.
 *
 * @param span span instance
 * @return timestamp in microseconds when the span has been started.
 *
 * @committed
 */
LIBCOUCHBASE_API
lcb_U64 lcbtrace_span_get_start_ts(lcbtrace_SPAN *span);

/**
 * Get finish timestamp of the span.
 *
 * @param span span instance
 * @return timestamp in microseconds when the span has been finished.
 *
 * @committed
 */
LIBCOUCHBASE_API
lcb_U64 lcbtrace_span_get_finish_ts(lcbtrace_SPAN *span);

/**
 * Check if the span is orphaned.
 *
 * Spans might be marked as orphaned, when the library has discarded
 * request structure without waiting for server response (e.g. on timeout).
 *
 * @param span span instance
 * @return non-zero if span is orphaned.
 *
 * @committed
 */
LIBCOUCHBASE_API
int lcbtrace_span_is_orphaned(lcbtrace_SPAN *span);

/**
 * Get operation code of the span.
 *
 * @param span span instance
 * @return operation code
 *
 * @committed
 */
LIBCOUCHBASE_API
const char *lcbtrace_span_get_operation(lcbtrace_SPAN *span);

/**
 *  Set the service for the span.
 *
 *  Threshold logging uses this to determine which threshold to apply when
 *  deciding whether or not a span has taken too long and should be logged.
 *  This only applies to outer spans.  See lcbtrace_span_set_is_outer for
 *  setting that.
 *
 * @param span the span to set the service on.
 * @param svc the desired service.
 * @return LCB_SUCCESS when successful
 *
 * @volatile
 */
LIBCOUCHBASE_API
lcb_STATUS lcbtrace_span_set_service(lcbtrace_SPAN *span, lcbtrace_SERVICE svc);

/**
 *  Get the service for the span.
 *
 *  Threshold logging uses this to determine which threshold to apply when
 *  deciding whether or not a span has taken too long and should be logged.
 *  This only applies to outer spans.  See lcbtrace_span_set_is_outer for
 *  setting that.
 *
 * @param span the span.
 * @param svc points to the the span's service, or to LCB_SERVICE__MAX if one is not set.
 * @return LCB_SUCCESS when successful.
 *
 * @volatile
 */
LIBCOUCHBASE_API
lcb_STATUS lcbtrace_span_get_service(lcbtrace_SPAN *span, lcbtrace_SERVICE *svc);

/**
 * @brief Get outer flag on span
 *
 * See @ref lcbtrace-span_get_is_outer for description of flag.
 * @param span span instance
 * @param outer pointer to the value of the flag.
 * @return LCB_SUCCESS when successful.
 *
 * @volatile
 */
LIBCOUCHBASE_API
lcb_STATUS lcbtrace_span_get_is_outer(lcbtrace_SPAN *span, int *outer);

/**
 * @brief Set outer flag on span
 *
 * When libcouchbase is passed a parent span with this flag set, it will
 * use it as its outer span for the operation, adding the appropriate tags.
 * However, it will _not_ finish the span when calling the callback, like
 * it does when it creates its own outer span.  The finish is up to the
 * caller, perhaps allowing for extra spans accounting for the processing
 * within the callback, or elsewhere.
 *
 * @param span the span to modify.
 * @param outer when set, span is considered an outer span, whose finish time
 *              will be managed by the caller.
 * @return LCB_SUCCESS when successful.
 *
 * @volatile
 */
LIBCOUCHBASE_API
lcb_STATUS lcbtrace_span_set_is_outer(lcbtrace_SPAN *span, int outer);

/**
 * @brief Get encoding flag on span
 *
 * See @ref lcbtrace-span_get_is_encoding for description of flag.
 * @param span span instance
 * @param encoding pointer to the value of the flag.
 * @return LCB_SUCCESS when successful.
 *
 * @volatile
 */
LIBCOUCHBASE_API
lcb_STATUS lcbtrace_span_get_is_encoding(lcbtrace_SPAN *span, int *encoding);

/**
 * @brief Set encoding flag on span
 *
 * When libcouchbase sees an encoding span has finished, it propogates the
 * duration up to the outer span.  This is output by the threshold logging tracer.
 *
 * @param span the span to modify.
 * @param encode when set, span is considered an encode span, whose duration will
 *               be output by the threshold logging tracer.
 * @return LCB_SUCCESS when successful.
 *
 * @volatile
 */
LIBCOUCHBASE_API
lcb_STATUS lcbtrace_span_set_is_encode(lcbtrace_SPAN *span, int encode);

/**
 * @brief Get dispatch flag on span
 *
 * See @ref lcbtrace-span_get_is_dispatch for description of flag.
 * @param span span instance
 * @param dispatch pointer to the value of the flag.
 * @return LCB_SUCCESS when successful.
 *
 * @volatile
 */
LIBCOUCHBASE_API
lcb_STATUS lcbtrace_span_get_is_dispatch(lcbtrace_SPAN *span, int *dispatch);

/**
 * @brief Set dispatch flag on span
 *
 * When a span is marked as being a dispatch span, libcouchbase will copy the
 * tags into the parent span as well.  This is used internally, for threshold
 * and orphan logging.  If an external external tracer is being used, where the
 * callbacks in the lcbtrace_TRACER is using the v1 callbacks, this copying of
 * the tags will not be done.
 *
 * @param span the span to modify.
 * @param dispatch when set, span is considered a dispatch span.
 * @return LCB_SUCCESS when successful.
 *
 * @volatile
 */
LIBCOUCHBASE_API
lcb_STATUS lcbtrace_span_set_is_dispatch(lcbtrace_SPAN *span, int dispatch);

#define LCBTRACE_OP_REQUEST_ENCODING "request_encoding"
#define LCBTRACE_OP_DISPATCH_TO_SERVER "dispatch"
#define LCBTRACE_OP_RESPONSE_DECODING "response_decoding"

#define LCBTRACE_OP_INSERT "insert"
#define LCBTRACE_OP_APPEND "append"
#define LCBTRACE_OP_COUNTER "counter"
#define LCBTRACE_OP_GET "get"
#define LCBTRACE_OP_GET_FROM_REPLICA "get_from_replica"
#define LCBTRACE_OP_INSERT "insert"
#define LCBTRACE_OP_OBSERVE_CAS "observe_cas"
#define LCBTRACE_OP_OBSERVE_CAS_ROUND "observe_cas_round"
#define LCBTRACE_OP_OBSERVE_SEQNO "observe_seqno"
#define LCBTRACE_OP_PREPEND "prepend"
#define LCBTRACE_OP_REMOVE "remove"
#define LCBTRACE_OP_REPLACE "replace"
#define LCBTRACE_OP_TOUCH "touch"
#define LCBTRACE_OP_UNLOCK "unlock"
#define LCBTRACE_OP_UPSERT "upsert"
#define LCBTRACE_OP_EXISTS "exists"
#define LCBTRACE_OP_LOOKUPIN "lookup_in"
#define LCBTRACE_OP_MUTATEIN "mutate_in"
#define LCBTRACE_OP_QUERY "query"
#define LCBTRACE_OP_ANALYTICS "analytics"
#define LCBTRACE_OP_SEARCH "search"
#define LCBTRACE_OP_VIEW "views"

#define LCBTRACE_TAG_SPAN_KIND "span.kind"
/**
 * Bucket name
 */
#define LCBTRACE_TAG_DB_INSTANCE "db.instance"
/**
 * The client's identifier string (the 'u' property in the updated HELLO request),
 * the same one that is shared with the server to identify the SDK.
 */
#define LCBTRACE_TAG_COMPONENT "db.couchbase.component"
/**
 * The unique ID of the operation
 */
#define LCBTRACE_TAG_OPERATION_ID "db.couchbase.operation_id"
/**
 * The service type, one of the following:
 * kv, view, n1ql, search, analytics
 */
#define LCBTRACE_TAG_SERVICE "db.couchbase.service"
#define LCBTRACE_TAG_SERVICE_KV "kv"
#define LCBTRACE_TAG_SERVICE_VIEW "views"
#define LCBTRACE_TAG_SERVICE_N1QL "query"
#define LCBTRACE_TAG_SERVICE_SEARCH "search"
#define LCBTRACE_TAG_SERVICE_ANALYTICS "analytics"

/**
 *  Connection id used when creating a connection against the cluster.
 *  Note this is only used in KV spans.
 */
#define LCBTRACE_TAG_LOCAL_ID "db.couchbase.local_id"

/**
 * The local socket ip address.
 * To be added to dispatch spans when the local socket is available.
 */
#define LCBTRACE_TAG_LOCAL_ADDRESS "net.host.name"
/**
 * The local socket port.
 * To be added to dispatch spans when the local socket is available.
 */
#define LCBTRACE_TAG_LOCAL_PORT "net.host.port"

/**
 * The remote socket ip address.
 * To be added to dispatch spans when the local socket is available.
 */
#define LCBTRACE_TAG_PEER_ADDRESS "net.peer.name"
/**
 * The remote socket port.
 * To be added to dispatch spans when the local socket is available.
 */
#define LCBTRACE_TAG_PEER_PORT "net.peer.port"
/**
 * The server duration, as reported in the server response.
 */
#define LCBTRACE_TAG_PEER_LATENCY "db.couchbase.server_duration"
/**
 * The scope used for this span.
 */
#define LCBTRACE_TAG_SCOPE "db.couchbase.scope"
/**
 * The collection used for this span.
 */
#define LCBTRACE_TAG_COLLECTION "db.couchbase.collection"
/**
 * The statement used in this span, when applicable.  This is set for
 * Query and Analytics.
 */
#define LCBTRACE_TAG_STATEMENT "db.statement"
/**
 * The operation for the span.  Set unless the db.statement key has been set.
 */
#define LCBTRACE_TAG_OPERATION "db.operation"
/**
 * The durability of the operation in this span, when applicable.
 */
#define LCBTRACE_TAG_DURABILITY "db.couchbase.durability"
/**
 * The number of retries performed in the span.
 */
#define LCBTRACE_TAG_RETRIES "db.couchbase.retries"

/**
 *  The system we are tracing
 */
#define LCBTRACE_TAG_SYSTEM "db.system"

/**
 * Transport used in trace.
 */
#define LCBTRACE_TAG_TRANSPORT "db.net.transport"
/**
 * Get ID of the span.
 *
 * @param span span instance
 * @return span ID
 *
 * @committed
 */
LIBCOUCHBASE_API
lcb_U64 lcbtrace_span_get_span_id(lcbtrace_SPAN *span);

/**
 * Get trace ID of the span.
 *
 * @param span span instance
 * @return trace ID
 *
 * @committed
 */
LIBCOUCHBASE_API
lcb_U64 lcbtrace_span_get_trace_id(lcbtrace_SPAN *span);

/**
 * Get parent span of the span.
 *
 * @param span span instance
 * @return parent span or NULL
 *
 * @committed
 */
LIBCOUCHBASE_API
lcbtrace_SPAN *lcbtrace_span_get_parent(lcbtrace_SPAN *span);

/**
 * Get value of the string tag of the span.
 *
 * @param span span instance
 * @param name name of the tag
 * @param value pointer to result string
 * @param nvalue pointer to length of the result string
 * @return LCB_SUCCESS if value exists and was written to result pointer
 *
 * @committed
 */
LIBCOUCHBASE_API
lcb_STATUS lcbtrace_span_get_tag_str(lcbtrace_SPAN *span, const char *name, char **value, size_t *nvalue);

/**
 * Get value of the integer tag of the span.
 *
 * @param span span instance
 * @param name name of the tag
 * @param value pointer to result
 * @return LCB_SUCCESS if value exists and was written to result pointer
 *
 * @committed
 */
LIBCOUCHBASE_API
lcb_STATUS lcbtrace_span_get_tag_uint64(lcbtrace_SPAN *span, const char *name, lcb_U64 *value);

/**
 * Get value of the double tag of the span.
 *
 * @param span span instance
 * @param name name of the tag
 * @param value pointer to result
 * @return LCB_SUCCESS if value exists and was written to result pointer
 *
 * @committed
 */
LIBCOUCHBASE_API
lcb_STATUS lcbtrace_span_get_tag_double(lcbtrace_SPAN *span, const char *name, double *value);

/**
 * Get value of the boolean tag of the span.
 *
 * @param span span instance
 * @param name name of the tag
 * @param value pointer to result
 * @return LCB_SUCCESS if value exists and was written to result pointer
 *
 * @committed
 */
LIBCOUCHBASE_API
lcb_STATUS lcbtrace_span_get_tag_bool(lcbtrace_SPAN *span, const char *name, int *value);

/**
 * Add string tag to span.
 *
 * @param span span instance
 * @param name name of the tag
 * @param value value of the tag (NUL-terminated)
 *
 * @committed
 */
LIBCOUCHBASE_API
void lcbtrace_span_add_tag_str(lcbtrace_SPAN *span, const char *name, const char *value);

/**
 * Add integer tag to span.
 *
 * @param span span instance
 * @param name name of the tag
 * @param value value of the tag
 *
 * @committed
 */
LIBCOUCHBASE_API
void lcbtrace_span_add_tag_uint64(lcbtrace_SPAN *span, const char *name, lcb_U64 value);

/**
 * Add double tag to span.
 *
 * @param span span instance
 * @param name name of the tag
 * @param value value of the tag
 *
 * @committed
 */
LIBCOUCHBASE_API
void lcbtrace_span_add_tag_double(lcbtrace_SPAN *span, const char *name, double value);

/**
 * Add boolean tag to span.
 *
 * @param span span instance
 * @param name name of the tag
 * @param value value of the tag. 0 if false, otherwise -- true.
 *
 * @committed
 */
LIBCOUCHBASE_API
void lcbtrace_span_add_tag_bool(lcbtrace_SPAN *span, const char *name, int value);

/**
 * Sets the tracing context for the command.
 *
 * @param cmd the command structure
 * @param ctx the lcbtrace_SPAN pointer
 *
 * @committed
 */
#define LCB_CMD_SET_TRACESPAN(cmd, span)                                                                               \
    do {                                                                                                               \
        (cmd)->pspan = span;                                                                                           \
    } while (0)

/**
 * @uncommitted
 */
typedef struct {
    void *state;
    void (*report)(void *state, lcbtrace_SPAN *span);
} ldcptrace_REPORTER;

/**
 * @} (Group: Tracing)
 */

#ifdef __cplusplus
}
#endif
#endif /* LCB_TRACING_H */
