# APAKER

APAKER is a custom packer employing the nanomites technique as an anti-debug and anti-dump protection. It's name comes from the Luxembourgish word for packer, however you pronounce it like you'd pronounce "a packer" in English.

## INSTALLATION

Right now, APAKER only works on Linux. To install it, just download the repository from github. The only other requirement is openssl.

## Usage
The packer has only been tested on C source files but it should work for any compiled language that supports inline assembly code, such as C++, Pascal or Rust. Using a different language probably makes the reverse engineering process even harder as you need to interpret two different flavors of generated assembly in one single executable.
### Preparation of source files 
The source files needs to be prepared so our packer knows where to implement the nanomites. If you want your code to be obfuscated, you need to put it inside a nanomite section. A nanomite section has a start marker and an end marker so the packer knows where it should add the obfuscation. To add the start marker, simply add the following inline assembly code to your source file: 
```assembly
movq $3405688917, %r15
C: __asm__("movq $3405688917, %r15");
``` 
To add the end marker, add the following assembly code to your source:
```assembly
movq $3405647957, %r15
C: __asm__("movq $3405647957, %r15");
```
However, there are a few rules on how you can layout these sections.
### Rules
Rule 1: Keep nanomites section inside a single function. While you can add as many sections as you want, you need to make sure they all stay within a single function. This is due to the fact that the code inside a nanomite section needs to be adjacent.

What doesn't work:
```C
int random_function() {
	__asm__("movq $3405688917, %r15");	//start
	int x = 1+1;
	return x;
}

int main() {
	random_function();
	int y = 2;
	__asm__("movq $3405647957, %r15");	//end
}
```
What works:
```C
int random_function() {
	__asm__("movq $3405688917, %r15");	//start
	int x = 1+1;
	__asm__("movq $3405647957, %r15");	//end
	return x;
}

int main() {
	random_function();
	__asm__("movq $3405688917, %r15");	//start
	int y = 2;
	__asm__("movq $3405647957, %r15");  //end
}
```
Rule 2: Make sure the start marker and end marker are placed in a way that the code flow can reach both of them.

What doesn't work:
```C
int  first_function()  {  
	__asm__("movq $3405688917, %r15"); 
	int x =  1+1;  
	return x;  
	__asm__("movq $3405647957, %r15");  //end  marker is not reached because function returns before
}

void second_function(int y) {
	__asm__("movq $3405688917, %r15"); 
	int z = y+1;
	if (y == 2) {
		printf("2\n");
		__asm__("movq $3405647957, %r15");  //end marker is not always reached because code flow depends on function arguments
	}
	else {
		printf("Not 2\n");
	}
}
```
What works:
```C
int  first_function()  {  
	__asm__("movq $3405688917, %r15"); 
	int x =  1+1;  
	__asm__("movq $3405647957, %r15"); //set end marker before return
	return x;  
}

void second_function(int y) {
	__asm__("movq $3405688917, %r15"); 
	int z = y+1;
	if (y == 2) {
		printf("2\n");
	}
	else {
		printf("Not 2\n");
	}
	__asm__("movq $3405647957, %r15");  //end marker is always reached
}
```
Rule 3: Make sure you close a section logically before opening a new one.

What doesn't work:
```C
int random_function() {
	__asm__("movq $3405688917, %r15"); // 2. another section gets opened
	int z = 3*3;
	__asm__("movq $3405647957, %r15"); // 3. new section gets closed
	return z;
}
int main() {
	__asm__("movq $3405688917, %r15"); // 1. section gets opened
	int x = 5;
	random_function()
	int u = 8;
	__asm__("movq $3405647957, %r15"); // 4. old section gets closed
	return 1;
}
```
What works:
```C
int random_function() {
	__asm__("movq $3405688917, %r15"); // 3. section gets opened
	int z = 3*3;
	__asm__("movq $3405647957, %r15"); // 4. section gets closed
	return z;
}
int main() {
	__asm__("movq $3405688917, %r15"); // 1. section gets opened
	int x = 5;
	__asm__("movq $3405647957, %r15"); // 2. section gets closed
	random_function()
	__asm__("movq $3405688917, %r15"); // 5. section gets opened
	int u = 8;
	__asm__("movq $3405647957, %r15"); // 6. section gets closed
	return 1;
}
```
If you follow these rules, you can make as many sections as you want. 
Note: If the sections are too small, the code inside might not be encrypted which weakens the protection a bit.
### Compiling the source file
After placing the sections, you can use any compiler to compile the source into an ELF. Make sure that the start and end markers can still be found in the compiled ELF, some compilers might try to remove or optimize them, which makes it impossible for the packer to locate the sections.
### Adding nanomites
After compiling your source files, you can use the add_nanomites.sh script to add the nanomites. It takes two arguments, first your unencrypted ELF that you just compiled and then a name for the output ELF containing the nanomites. You can now take the output ELF and run it anywhere you want.
## EXAMPLES
There's an example C script in the examples folder as well as a shell script that compiles the script into different types of ELF files (with PIE and without).
## Science behind the tool
The nanomite technique has been used by the Armadillo Protector. You can find out more about the technique on [apriorit](https://www.apriorit.com/white-papers/293-nanomite-technology) and on [infosec institute](https://resources.infosecinstitute.com/topic/anti-memory-dumping-techniques/). You can also take a look at the APAKER source to better understand how nanomites work. I tried to add explaining comments so it's easier to follow the code.
## Extension
You could definitely make this packer better by encrypting the nanomite details or adding a few fake nanomites. I might add this functionaility in the future.
## Sources
APAKER has been inspired by the **debug_me_if_you_can** challenge from justCTF 2020, made by the justCatTheFish team. At this point it's still available on [2020.justctf.team](https://2020.justctf.team/challenges/2). The exact implementation however differs considerably. In contrast to the CTF challenge, APAKER packed executables also run on their own and can be distributed in a single executable.


