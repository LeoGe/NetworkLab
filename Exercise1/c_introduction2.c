#include<stdio.h>
#include<string.h>
#include<stdlib.h>

void main(){
    double taxrate = 7.3, discountrate;
    char buyer[100], seller[100];

    double* tmpPtr = 0;
    tmpPtr = &taxrate;
    printf("taxrate: %f\n", *tmpPtr);
    discountrate = *tmpPtr;
    printf("discountrate: %f\n", discountrate);
    printf("Adress of taxrate %p\n",tmpPtr);
    //Nope
    strcpy(buyer, "Ganz viele coole Menschen\0");
    strcpy(seller, buyer);
    if(strcmp(seller, buyer)==0){
        printf("Seller and buyer are equal\n");
    }
    strcat(buyer, seller);
    printf("Buyer: %s with length: %i\n", buyer, strlen(buyer));

}
