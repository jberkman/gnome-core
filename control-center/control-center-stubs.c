/*
 * This file was generated by orbit-idl - DO NOT EDIT!
 */

#include <string.h>
#include "control-center.h"

#define GET_ATOM(x) ({ GIOP_RECV_BUFFER(_ORBIT_recv_buffer)->decoder(&x, (GIOP_RECV_BUFFER(_ORBIT_recv_buffer)->cur), sizeof(x)); GIOP_RECV_BUFFER(_ORBIT_recv_buffer)->cur += sizeof(x); })
/***************** Begin module GNOME ***************/
CORBA_short
GNOME_control_panel_cpo_request_id(GNOME_control_panel _obj,
				   CORBA_char * cookie,
				   CORBA_Environment * ev)
{
   GIOP_unsigned_long _ORBIT_request_id;
   GIOPSendBuffer *_ORBIT_send_buffer;
   GIOPRecvBuffer *_ORBIT_recv_buffer;
   static const struct {
      CORBA_unsigned_long len;
      char opname[15];
   } _ORBIT_operation_name_data = {
      15, "cpo_request_id"
   };
   static const struct iovec _ORBIT_operation_vec =
   {(gpointer) & _ORBIT_operation_name_data, 19};
   CORBA_short _ORBIT_retval;

   _ORBIT_request_id = giop_get_request_id();
   _ORBIT_send_buffer =
      giop_send_request_buffer_use(_obj->connection, NULL,
		     _ORBIT_request_id, CORBA_TRUE, &(_obj->object_key_vec),
			   &_ORBIT_operation_vec, &default_principal_iovec);

   /* marshal parameter cookie */
   {
      GIOP_unsigned_long len = cookie ? (strlen(cookie) + 1) : 0;

      giop_send_buffer_append_mem_indirect_a(GIOP_SEND_BUFFER(_ORBIT_send_buffer), &len, sizeof(len));
      if (cookie)
	 giop_message_buffer_append_mem(GIOP_MESSAGE_BUFFER(_ORBIT_send_buffer), cookie, len);
   }

   giop_send_buffer_write(_ORBIT_send_buffer);
   giop_send_buffer_unuse(_ORBIT_send_buffer);
   _ORBIT_recv_buffer = giop_recv_reply_buffer_use(_ORBIT_request_id, TRUE);
   if (_ORBIT_recv_buffer == NULL || _ORBIT_recv_buffer->message_buffer.message_header.message_type != GIOP_REPLY) {
      CORBA_exception_set_system(ev, ex_CORBA_COMM_FAILURE, CORBA_COMPLETED_MAYBE);
      if (_ORBIT_recv_buffer)
	 giop_recv_buffer_unuse(_ORBIT_recv_buffer);
      return;
   }
   if (_ORBIT_recv_buffer->message.u.reply.reply_status != GIOP_NO_EXCEPTION) {
      ORBit_handle_exception(_ORBIT_recv_buffer, ev, NULL);
      giop_recv_buffer_unuse(_ORBIT_recv_buffer);
      return;
   }
   if (giop_msg_conversion_needed(GIOP_MESSAGE_BUFFER(_ORBIT_recv_buffer))) {
/* demarshal return value */
      GIOP_RECV_BUFFER(_ORBIT_recv_buffer)->cur = ALIGN_ADDRESS(GIOP_RECV_BUFFER(_ORBIT_recv_buffer)->cur, 2);
      GET_ATOM(_ORBIT_retval);
   } else {
/* demarshal return value */
      GIOP_RECV_BUFFER(_ORBIT_recv_buffer)->cur = ALIGN_ADDRESS(GIOP_RECV_BUFFER(_ORBIT_recv_buffer)->cur, 2);
      _ORBIT_retval = *((CORBA_short *) GIOP_RECV_BUFFER(_ORBIT_recv_buffer)->cur);
      GIOP_RECV_BUFFER(_ORBIT_recv_buffer)->cur += sizeof(CORBA_short);
   }

   giop_recv_buffer_unuse(_ORBIT_recv_buffer);
   ev->_major = CORBA_NO_EXCEPTION;
   return _ORBIT_retval;
}

void
GNOME_control_panel_cpo_register(GNOME_control_panel _obj,
				 CORBA_char * ior,
				 CORBA_short cpo_id,
				 CORBA_Environment * ev)
{
   GIOP_unsigned_long _ORBIT_request_id;
   GIOPSendBuffer *_ORBIT_send_buffer;
   GIOPRecvBuffer *_ORBIT_recv_buffer;
   static const struct {
      CORBA_unsigned_long len;
      char opname[13];
   } _ORBIT_operation_name_data = {
      13, "cpo_register"
   };
   static const struct iovec _ORBIT_operation_vec =
   {(gpointer) & _ORBIT_operation_name_data, 17};

   _ORBIT_request_id = giop_get_request_id();
   _ORBIT_send_buffer =
      giop_send_request_buffer_use(_obj->connection, NULL,
		     _ORBIT_request_id, CORBA_TRUE, &(_obj->object_key_vec),
			   &_ORBIT_operation_vec, &default_principal_iovec);

   /* marshal parameter ior */
   {
      GIOP_unsigned_long len = ior ? (strlen(ior) + 1) : 0;

      giop_send_buffer_append_mem_indirect_a(GIOP_SEND_BUFFER(_ORBIT_send_buffer), &len, sizeof(len));
      if (ior)
	 giop_message_buffer_append_mem(GIOP_MESSAGE_BUFFER(_ORBIT_send_buffer), ior, len);
   }

   /* marshal parameter cpo_id */
   giop_message_buffer_append_mem_a(GIOP_MESSAGE_BUFFER(_ORBIT_send_buffer), &cpo_id, sizeof(cpo_id));

   giop_send_buffer_write(_ORBIT_send_buffer);
   giop_send_buffer_unuse(_ORBIT_send_buffer);
   _ORBIT_recv_buffer = giop_recv_reply_buffer_use(_ORBIT_request_id, TRUE);
   if (_ORBIT_recv_buffer == NULL || _ORBIT_recv_buffer->message_buffer.message_header.message_type != GIOP_REPLY) {
      CORBA_exception_set_system(ev, ex_CORBA_COMM_FAILURE, CORBA_COMPLETED_MAYBE);
      if (_ORBIT_recv_buffer)
	 giop_recv_buffer_unuse(_ORBIT_recv_buffer);
      return;
   }
   if (_ORBIT_recv_buffer->message.u.reply.reply_status != GIOP_NO_EXCEPTION) {
      ORBit_handle_exception(_ORBIT_recv_buffer, ev, NULL);
      giop_recv_buffer_unuse(_ORBIT_recv_buffer);
      return;
   }
   giop_recv_buffer_unuse(_ORBIT_recv_buffer);
   ev->_major = CORBA_NO_EXCEPTION;
}

void
GNOME_control_panel_quit(GNOME_control_panel _obj,
			 CORBA_char * cookie,
			 CORBA_Environment * ev)
{
   GIOP_unsigned_long _ORBIT_request_id;
   GIOPSendBuffer *_ORBIT_send_buffer;
   GIOPRecvBuffer *_ORBIT_recv_buffer;
   static const struct {
      CORBA_unsigned_long len;
      char opname[5];
   } _ORBIT_operation_name_data = {
      5, "quit"
   };
   static const struct iovec _ORBIT_operation_vec =
   {(gpointer) & _ORBIT_operation_name_data, 9};

   _ORBIT_request_id = giop_get_request_id();
   _ORBIT_send_buffer =
      giop_send_request_buffer_use(_obj->connection, NULL,
		     _ORBIT_request_id, CORBA_TRUE, &(_obj->object_key_vec),
			   &_ORBIT_operation_vec, &default_principal_iovec);

   /* marshal parameter cookie */
   {
      GIOP_unsigned_long len = cookie ? (strlen(cookie) + 1) : 0;

      giop_send_buffer_append_mem_indirect_a(GIOP_SEND_BUFFER(_ORBIT_send_buffer), &len, sizeof(len));
      if (cookie)
	 giop_message_buffer_append_mem(GIOP_MESSAGE_BUFFER(_ORBIT_send_buffer), cookie, len);
   }

   giop_send_buffer_write(_ORBIT_send_buffer);
   giop_send_buffer_unuse(_ORBIT_send_buffer);
   _ORBIT_recv_buffer = giop_recv_reply_buffer_use(_ORBIT_request_id, TRUE);
   if (_ORBIT_recv_buffer == NULL || _ORBIT_recv_buffer->message_buffer.message_header.message_type != GIOP_REPLY) {
      CORBA_exception_set_system(ev, ex_CORBA_COMM_FAILURE, CORBA_COMPLETED_MAYBE);
      if (_ORBIT_recv_buffer)
	 giop_recv_buffer_unuse(_ORBIT_recv_buffer);
      return;
   }
   if (_ORBIT_recv_buffer->message.u.reply.reply_status != GIOP_NO_EXCEPTION) {
      ORBit_handle_exception(_ORBIT_recv_buffer, ev, NULL);
      giop_recv_buffer_unuse(_ORBIT_recv_buffer);
      return;
   }
   giop_recv_buffer_unuse(_ORBIT_recv_buffer);
   ev->_major = CORBA_NO_EXCEPTION;
}

/***************** End module GNOME ***************/
