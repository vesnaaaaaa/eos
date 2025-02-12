#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>



// str je string  char-> je adresa u memoriji tipa char 

//char* je niz u c -> adresa u memoriji tipa char


// strrev() funkcija sluzi da se obrne dati string 

char *strrev(char *str) {
    int len = strlen(str);
  
      // Temporary char array to store the
	  
    // reversed string     **** nije mi jasno
	// malloc sluzi za alociranje memorije
	
    char *rev = (char *)malloc
      (sizeof(char) * (len + 1));
  
    // String koji se cita od nazad
	
    for (int i = 0; i < len; i++) {
        rev[i] = str[len - i - 1];
    }
    rev[len] = '\0';
    return rev;
}

void isPalindrome(char *str) {

    // Obrnuti string, proveravanje reci unazad!
	// strrev() funkcija sluzi da se obrne dati string 
	
    char *rev = strrev(str);

    // Check if the original and reversed
	
      // da li su stringovi isti
	  
	  //In C, strcmp() is a built-in library function used to compare two strings lexicographically.
	  //It takes two strings (array of characters) as arguments,
	  //compares these two strings lexicographically,
	 // and then returns 0,1, or -1 as the result.
	  
    if (strcmp(str, rev) == 0)
        printf("%s je palindrom.\n",str);
    else
        printf("%s nije palindrom.\n",str);
}

int main() {
    
    // Funkcija sa kojom proveravam da li je neka rec palindrom, ili ta rec nije palindrom!!!
	//znaci mi u zagradi imamo neki argument koji ubacujemo u funkciju i ona nam proverava da li je uslov za resenje ispunjen
	  
    isPalindrome("teret");
    isPalindrome("bazar");

    return 0;
}