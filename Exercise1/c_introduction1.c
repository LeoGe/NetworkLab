#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Nr 1
struct route{
    int routeID;
    char descrp[25];
};
//Nr 2
typedef struct route Route;

//Nr4
void readRoute(Route* route){
    char buf[25];
    char* b = buf;
    size_t buf_size = sizeof(buf);
    size_t line_size = 0;
    
    printf("Please type a route_id...\n");
    line_size = getline(&b, &buf_size, stdin);
    buf[line_size-1] = '\0';
    route->routeID = atoi(buf);
    
    printf("Please type a description...\n");
    line_size = getline(&b, &buf_size, stdin);
    buf[line_size-1] = '\0';
    strcpy(route->descrp, buf);

    printf("You have created the following route:\nRoute Id: %i\nRoute description %s\n",route->routeID, route->descrp);

}

void main(){
    //Nr 3
    Route route1;
    Route longRoutes[10];
    Route* routePtr = 0;    
    //Nr 4
    readRoute(&route1);
    //Nr 5
    longRoutes[2] = route1;
    //Nr 6
    routePtr = longRoutes;
}
