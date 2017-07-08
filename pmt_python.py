import pmt
import socket

pmt_to_send  = pmt.make_dict()


attributes = pmt.make_dict()
attributes = pmt.dict_add(attributes, pmt.string_to_symbol("center_freq"),pmt.from_double(15e6))
attributes = pmt.dict_add(attributes, pmt.string_to_symbol("occ"),pmt.from_double(0.0134))
attributes = pmt.dict_add(attributes, pmt.string_to_symbol("bandwidth"),pmt.from_double(2e6))

command = pmt.make_dict()
command = pmt.dict_add(command,pmt.string_to_symbol("table"),pmt.string_to_symbol("spectruminfo"))
command = pmt.dict_add(command, pmt.string_to_symbol("attributes"),attributes)

pmt_to_send = pmt.dict_add(pmt_to_send, pmt.string_to_symbol("INSERT"),command)

serialized_pmt = pmt.serialize_str(pmt_to_send)

#print pmt_to_send

UDP_IP = "127.0.0.1"
UDP_PORT = 6000

sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM) 
sock.sendto(serialized_pmt, (UDP_IP, UDP_PORT))


#print serialized_pmt