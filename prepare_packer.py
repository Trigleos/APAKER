import random
import hashlib


def encrypt_code(data,encrypted_start):
	seed = random.randint(0,4294967295)
	#hashvalue = hashlib.md5(seed.to_bytes(8,"big"))
	hashvalue = hashlib.md5(str(seed).encode())
	xor = int.from_bytes(hashvalue.digest()[-8:],"big")
	second_xor = int.from_bytes(data[encrypted_start:encrypted_start+8],"big")
	print(hex(xor))
	print(hex(second_xor))
	encrypted_code = second_xor ^ xor
	encrypted_code = encrypted_code.to_bytes(8,"big")
	for i in range(8):
		data[encrypted_start + i] = encrypted_code[i]
	return (encrypted_start,seed)
		

def implement_nanomites(data,index,end_index):
	nanomites = []
	available_space = end_index-index-8
	code_steals = available_space // 16
	data[index] = 0xcc
	encrypted_start = index + 8
	for i in range(code_steals):
		nanomites.append(encrypt_code(data,encrypted_start))
		encrypted_start += 16
	return nanomites


with open("example_pie","rb") as f:
	data = bytearray(f.read())

nanomites = []
for i in range(len(data)):
	if data[i:i+6] == b"\x49\xbf\x55\xb0\xfe\xca":
		index = i
		end_index = index + data[i:].find(b"\x49\xbf\x55\x10\xfe\xca")
		nanomites.extend(implement_nanomites(data,index,end_index))
		
print(nanomites)
	
with open("output_nano_pie","wb") as f:
	f.write(data)
		

