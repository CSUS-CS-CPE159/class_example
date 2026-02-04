/* 
 * main.c -- Display Hello World 
 */

int main(){
    // Display "hello, world" in the first row
    unsigned short *vga_base = (unsigned short *)0xB8000;	
    *(vga_base + 0) = (unsigned short )0x0700 + 'h';
    *(vga_base + 1) = (unsigned short )0x0700 + 'e';
    *(vga_base + 2) = (unsigned short )0x0700 + 'l';
    *(vga_base + 3) = (unsigned short )0x0700 + 'l';
    *(vga_base + 4) = (unsigned short )0x0700 + 'o';
    *(vga_base + 5) = (unsigned short )0x0700 + ',';
    *(vga_base + 6) = (unsigned short )0x0700 + 'w';
    *(vga_base + 7) = (unsigned short )0x0700 + 'o';
    *(vga_base + 8) = (unsigned short )0x0700 + 'r';
    *(vga_base + 9) = (unsigned short )0x0700 + 'l';
    *(vga_base + 10) = (unsigned short )0x0700 + 'd';
    *(vga_base + 11) = (unsigned short )0x0700 + '!';
    char array[] = "hello world!";
    // Display "hello, world" in the second row 
    for (int i = 0; i < 12; i++)
        *(vga_base + 80 + i) = (unsigned short)0xB400 + array[i];
    return 0;
}
