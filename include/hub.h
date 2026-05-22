#ifndef __HUB_H__
#define __HUB_H__

typedef struct hub{
    int state;
    int switches;
    int registry[]; 
}hub;

hub createHub();


#endif