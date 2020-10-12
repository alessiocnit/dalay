#ifndef PARSER_HIDDEN_STRUCTURES_H
#define  PARSER_HIDDEN_STRUCTURES_H

#include "defs.h"

/*****************************************************************************/
/* 03/03/10                                                                  */
/* STRUCT DOMAIN_BLOCK: NODE_BLOCK DATA                                      */
/* By Cialo & Wamba @ SSSUP 						     */
/*****************************************************************************/
struct domain_block
{
    int ID;  /* ID TAKEN FROM FILE              */
    char type[STRING_LENGTH];
    char name[STRING_LENGTH]; /* NAME OF THE NODE */

    int PCE_ID; /*ID of the domain PCE (-1 means no PCE in the domain) */
    int NodeIDs_List[NMAX];
};

/*****************************************************************************/
/* 03/23/01                                                                  */
/* STRUCT NODE_BLOCK: NODE_BLOCK DATA                                        */
/*****************************************************************************/
struct node_block
{
    float xpos,ypos; /* COORDINATES            */
    int ID;  /* ID TAKEN FROM FILE              */
    char type[STRING_LENGTH]; /* TYPE OF THE NODE */
    char name[STRING_LENGTH]; /* NAME OF THE NODE */
    float trx_bitrate;  /* TRANSMISSION BIT RATE */
    float rec_sensitivity; /* RECEIVER SENSITIVITY */
    float ber; /* BIT ERROR RATE */
    float delay; /*DELAY: TIME UNIT TBD */
    float jitter; /* JITTER: TIME UNIT TBD */
    float bw; /* BANDWIDTH: UNIT TBD */
    float mtbf; /* MEAN TIME BETWEEN FAILURE: TIME UNIT TBD */
    float mttr; /* MEAN TIME TO REPAIR: UNIT TBD */
    char op_type[STRING_LENGTH]; /* OPERATIONAL TYPE = "NORMAL' */
    /* others TBD     */
    char status[STRING_LENGTH]; /* STATUS = "WORKING" */
    /* "NON_WORKING" */
    /* other = TBD */
};



/*****************************************************************************/
/* 03/23/01                                                                  */
/* STRUCT LINK_BLOCK: UNIDIRECTIONAL LINK                                    */
/*****************************************************************************/
struct link_block
{
    char type[STRING_LENGTH];
    char name[STRING_LENGTH];
    char from[STRING_LENGTH];
    char to[STRING_LENGTH];
    int ID;            /* LINK ID */
    float distance;    /* LENGTH OF THE LINK */
    /* Added by Marco Ghizzi */
    float maxbyte;     /* DIMENSION OF THE INPUT BUFFERS (OUT BUFFER FOR THE FEEDING NODE) */
    float num_channel; /* NUMBER OF CHANNELS IN ONE LINK (CAN BE USED FOR THE NUMBER OF WAVELENGTHS) */
    /* end */
    float ber;         /* BIT ERROR RATE */
    float delay;       /* DELAY: UNIT TBD */
    float jitter;      /* JITTER: UNIT TBD */
    float bw;          /* BANDWIDTH */
    float mfp;         /* ????? */
    int fiber_ratio;   /* FIBER RATIO NUMBER OF FIBERS? */
    struct fiber_list *fiber_array;
    float mtbf,mttr;
    float bitrate;          /* BITRATE - Marco Ghizzi added this line*/
};

/*****************************************************************************/
/* 03/23/01                                                                  */
/* STRUCT FIBER_LIST: DATA IN THE LINK_BLOCK TO DESCRIBE EACH ELEMENT        */
/* IN THE FIBER LIST                                                         */
/*****************************************************************************/
struct fiber_list
{
    char type[STRING_LENGTH];
    char name[STRING_LENGTH];
    char status[STRING_LENGTH];
    int ID;
    char op_type[STRING_LENGTH]; /* op_type == 0 -> NORMAL */
    /* op_type == 1 -> TBD */
    int lambda_ratio; /* NUMBER OF LAMBDAS ON THE FIBER? */
    struct lambda_elem *lambda_array;
    int amplifier_ratio; /* NUMBER OF AMPLIFIERS ALONG THE FIBER */
    struct amplifier_elem *amply_array;
};

/**************************************************************************/
/* 04/18/01                                                               */
/* STRUCT LAMBDA_ELEM USED TO BUILD ARRAY LAMBDA_ARRAY IN STRUCT          */
/* FIBER_LIST                                                             */
/**************************************************************************/
struct lambda_elem
{
    char lambda[STRING_LENGTH];
    int lambdaID;
    char op_type[STRING_LENGTH];
    char status[STRING_LENGTH];
};

/**************************************************************************/
/* 04/18/01                                                               */
/* STRUCT AMPLIFIER_ELEM USED TO BUILD ARRAY AMPLY_ARRAY IN STRUCT        */
/* FIBER_LIST                                                             */
/**************************************************************************/
struct amplifier_elem
{
    char amplifier_type[STRING_LENGTH]; /* TYPE OF APMLIFIER, I.E., */
    /* SOA, RAMAN, ETC. */
    char amplifier_name[STRING_LENGTH];
    float position; /* POSITION OF THE I-th AMPLIFIER */
};

/*****************************************************************************/
/* 05/03/01                                                                  */
/* STRUCT DEMAND_BLOCK: DEMAND_BLOCK DATA                                    */
/*****************************************************************************/
struct demand_block
{
    char name[STRING_LENGTH]; /* NAME OF THE DEMAND */
    char type[STRING_LENGTH]; /* TYPE OF THE DEMAND i.e. traffic*/
    char source[STRING_LENGTH]; /* NAME OF THE SOURCE NODE */
    char destination[STRING_LENGTH]; /* NAME OF THE DESTINATION NODE */
    float demand;  /* DEMAND AMOUNT */
    char demand_type[STRING_LENGTH]; /* TYPE OF THE TRAFFIC DEMAND */
    int demand_class;  /* DEMAND CLASS */
    // TO BE ADDED  struct lightpath_list; /* LIST OF LIGHTPATHS */
};

#endif
