import random
import hashlib
import sys


def encrypt_code(data,encrypted_start):	#encrypt 8 bytes of our source ELF
	seed = random.randint(0,2147483647)	#generate random seed for md5
	
	hashvalue = hashlib.md5(str(seed).encode())
	xor = int.from_bytes(hashvalue.digest()[-8:],"big")	#generare first xor value, the last 8 bytes of the md5 value
	
	second_xor = int.from_bytes(data[encrypted_start:encrypted_start+8],"big")	#second xor value is our unencrypted code
	
	encrypted_code = second_xor ^ xor
	encrypted_code = encrypted_code.to_bytes(8,"big")	#Encrypted code is unencrypted code xor last 8 bytes of md5 value
	
	for i in range(8):
		data[encrypted_start + i] = encrypted_code[i]		#put encrypted code into source ELF 
	return (encrypted_start,seed)		#return offset and seed for future decryption
		

def implement_nanomites(data,index,end_index):	#implement nanomites in one marked section
	nanomites = []
	
	available_space = end_index-index-10	#Doesn't make sense to encrypt our markers
	code_steals = available_space // 16	#one code steal = 8 bytes encrypted, 8 bytes not encrypted
	
	data[index] = 0xcc	#add breakpoint so child traps to parent (Decrypting routine)
	data[end_index] = 0xcc	#add breakpoint so child traps to parent (Encrypting routine)
	
	encrypted_start = index + 10 #Start encrypting after our marker
	for i in range(code_steals):
		nanomites.append(encrypt_code(data,encrypted_start))	#encrypt 8 bytes and add details to nanomites list
		encrypted_start += 16	#skip the next 8 bytes
	return nanomites

def dump_nanomites(nanomites, filename):	#simply dump implemented nanomites details, format:
	with open(filename,"w") as f:		#number_of_nanomites
		length = len(nanomites)	#physical_offset:md5_seed
		f.write(str(length)+"\n")	#...
		for nanomite in nanomites:
			f.write(str(nanomite[0])+":"+str(nanomite[1])+"\n")

def add_nanomites(input_filename,output_filename):  #top level function that reads in ELF and outputs the results
	with open(input_filename,"rb") as f:	#Read in compiled ELF file that contains the nanomite markers
		data = bytearray(f.read())

	nanomites = []
	for i in range(len(data)):
		if data[i:i+6] == b"\x49\xbf\x55\xb0\xfe\xca":	#Search nanomite start marker
			index = i
			end_index = index + data[i:].find(b"\x49\xbf\x55\x10\xfe\xca")	#Search corresponding nanomite end marker
			nanomites.extend(implement_nanomites(data,index,end_index))	#implement nanomite section
			
	dump_nanomites(nanomites,"resc/nanomites_dump")	#dump nanomite details to text file
		
	with open(output_filename,"wb") as f:	#Output ELF containing nanomites
		f.write(data)


if len(sys.argv) < 2:
	print("Please provide the name of the ELF file you want to add the nanomites to as an argument")
else:		
	add_nanomites(sys.argv[1],"resc/nanomites_encrypted")
		

