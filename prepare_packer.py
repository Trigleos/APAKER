import random
import hashlib


def encrypt_code(data,encrypted_start):
	seed = random.randint(0,2147483647)
	#hashvalue = hashlib.md5(seed.to_bytes(8,"big"))
	hashvalue = hashlib.md5(str(seed).encode())
	xor = int.from_bytes(hashvalue.digest()[-8:],"big")
	second_xor = int.from_bytes(data[encrypted_start:encrypted_start+8],"big")
	print("Normal_code:",hex(second_xor))
	encrypted_code = second_xor ^ xor
	encrypted_code = encrypted_code.to_bytes(8,"big")
	print("Encrypted_code:",encrypted_code.hex())
	for i in range(8):
		data[encrypted_start + i] = encrypted_code[i]
	return (encrypted_start,seed)
		

def implement_nanomites(data,index,end_index):
	nanomites = []
	available_space = end_index-index-8
	code_steals = available_space // 16
	data[index] = 0xcc
	data[end_index] = 0xcc
	print(hex(index))
	print(hex(end_index))
	encrypted_start = index + 8
	for i in range(code_steals):
		nanomites.append(encrypt_code(data,encrypted_start))
		encrypted_start += 16
	return nanomites

def dump_nanomites(nanomites, filename):
	with open(filename,"w") as f:
		length = len(nanomites)
		f.write(str(length)+"\n")
		for nanomite in nanomites:
			f.write(str(nanomite[0])+":"+str(nanomite[1])+"\n")


with open("example_pie","rb") as f:
	data = bytearray(f.read())

nanomites = []
for i in range(len(data)):
	if data[i:i+6] == b"\x49\xbf\x55\xb0\xfe\xca":
		index = i
		end_index = index + data[i:].find(b"\x49\xbf\x55\x10\xfe\xca")
		nanomites.extend(implement_nanomites(data,index,end_index))
		
dump_nanomites(nanomites,"nanomites_dump")
	
with open("output_nano_pie","wb") as f:
	f.write(data)
		

