
#include <stdlib.h>
#include <string.h>
#include <values.h>
#include <vector>

#include "parser_data.h"
#include "parser_hidden_structures.h"
#include "parser.h"

/****************************************************************************/
/* 06/26/01                                                                 */
/* FUNCTION READ_INPUT_FILE: READ THE FILE AND LOAD THE DATA STRUCUTRES     */
/****************************************************************************/
int read_input_file (
    list <struct domain_block> *l_domain,	// list of domain
    list <struct node_block> *l_node,    	// list of nodes
    list <struct link_block> *l_link,    	// list of links
    list <struct demand_block> *l_demand,	// list of demands
    FILE *input_stream,			// pointer to the actual position of the file
    int *n_domain,	// # of domain
    int *n_node,	// # of nodes
    int *n_link,	// # of links,
    int *n_demand,	// # of links demands
    int *n_classe	// # of classes
)
{
    struct domain_block domain, newdomain;		// return the node structure or null
    struct node_block node, newnode;		// return the node structure or null
    struct link_block link,newlink;			// return the link structure or null
    struct demand_block demand,newdemand;

    int x;
    int block_flag;
    int line_number=0;

    *n_classe=0;

    init_link_main(&link);

    while ((block_flag=get_next_line(input_stream,&line_number))!=EOF)
        switch (block_flag) {
        case DOMAIN_BLOCK: {
            do {
                x=read_domain_block((&domain),input_stream,&line_number);
                if (x!=END_OF_DOMAIN_BLOCK) {
                    (*l_domain).push_back(domain);
                    (*n_domain) ++;
                }
            } while (x!=END_OF_DOMAIN_BLOCK);
            break;
        }
        case NODE_BLOCK: {
            do {
                x=read_node_block((&node),input_stream,&line_number);
                if (x!=END_OF_NODE_BLOCK) {
                    (*l_node).push_back(node);
                    (*n_node) ++;
                }
            } while (x!=END_OF_NODE_BLOCK);
            break;
        }
        case LINK_BLOCK: {
            do {
                x=read_link_block((&link),input_stream,&line_number);
                if (x!=END_OF_LINK_BLOCK) {
                    (*l_link).push_back(link);
                    (*n_link) ++;
                }
            } while (x!=END_OF_LINK_BLOCK);
            break;
        }
        case DEMAND_BLOCK: {
            do {
                x=read_demand_block((&demand),input_stream,&line_number);
                if (x!=END_OF_DEMAND_BLOCK) {
                    (*l_demand).push_back(demand);
                    (*n_demand)++;
                    if ((demand.demand_class + 1 ) > (*n_classe))
                        (*n_classe) = demand.demand_class + 1;
                }
            } while (x!=END_OF_DEMAND_BLOCK);
            break;
        }
        }
    return 0;
}

/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTION IS_COMMENT: RETURN YES_PARSER IF LINE IS A COMMENT              */
/****************************************************************************/
int is_comment(char str[]) {
    int i = 0;
    while (str[i] == ' ')
        i++;
    switch (str[i])
    {
    case '/':
    {
        if (str[1] == '/')
            return (YES_PARSER);
        else
            return (NO_PARSER);
//	break;
    }
    case '\t':
    {
        return (YES_PARSER);
//	break;
    }
    case '\n':
    {
        return (YES_PARSER);
//	break;
    }
    case '\0':
    {
        return (YES_PARSER);
//	break;
    }
    case '{':
    {
        return (YES_PARSER);
//	break;
    }
    default:
        return (NO_PARSER);
    }

}



/****************************************************************************/
/* 04/26/01                                                                 */
/* FUNCTION GET_NEXT_LINE: READ A LINE FROM INPT FILE AND RETURNS A CODE    */
/* IDENTIFYING THE LINE                                                     */
/****************************************************************************/
int
get_next_line (FILE * input_stream, int *pline_number)
{
    int sscanf_value;
    char *pline;
    char line[STRING_LENGTH], str[STRING_LENGTH];
    do
    {
        pline = fgets (line, STRING_LENGTH + 1, input_stream);
        if (pline == NULL)
            return (EOF);
        (*pline_number)++;
        sscanf_value = sscanf (line, "%s", str);
        if ((sscanf_value == 0) || (sscanf_value == EOF))
            strcpy (str, line);
    }
    while (is_comment (str) == YES_PARSER);
    if (strcmp (str, "[DOMAIN_BLOCK]") == 0)
        return (DOMAIN_BLOCK);
    if (strcmp (str, "[NODE_BLOCK]") == 0)
        return (NODE_BLOCK);
    if (strcmp (str, "[LINK_BLOCK]") == 0)
        return (LINK_BLOCK);
    if (strcmp (str, "[DEMAND_BLOCK]") == 0)
        return (DEMAND_BLOCK);
    return (NOT_OK_PARSER);
}


/****************************************************************************/
/* 04/25/01                                                                 */
/* FUNCTION FREE_LAMBDA_ARRAY                                               */
/****************************************************************************/
void
free_lambda_array (struct lambda_elem *array)
{
    free (array);
}

/****************************************************************************/
/* 04/25/01                                                                 */
/* FUNCTION FREE_AMPLY_ARRAY                                                */
/****************************************************************************/
void
free_amply_array (struct amplifier_elem *array)
{
    free (array);
}

/***************************************************************************/
/* 03/04/10                                                             */
/* FUNCTION INIT_DOMAIN: INITIALIZE DOMAIN VALUES                       */
/* By Cialo @ SSSUP 							*/
/***************************************************************************/
int init_domain (struct domain_block *pdomain)
{
    pdomain->ID = -1;
    strcpy (pdomain->name, "\0");
    pdomain->PCE_ID = -1;
    for (int i=0; i<NMAX; i++) {
        pdomain->NodeIDs_List[i] = -1;
    }
    return (OK_PARSER);
}

/***************************************************************************/
/* 04/25/01                                                                */
/* FUNCTION INIT_NODE: INITIALIZE NODE VALUES                              */
/***************************************************************************/
int
init_node (struct node_block *pnode)
{
    pnode->xpos = pnode->ypos = MAXFLOAT;
    pnode->ID = -1;
    strcpy (pnode->name, "\0");
    strcpy (pnode->type, "\0");
    pnode->trx_bitrate = pnode->rec_sensitivity = -1.0;
    pnode->ber = pnode->delay = -1.0;
    pnode->jitter = pnode->bw = -1.0;
    pnode->mtbf = pnode->mttr = -1.0;
    strcpy (pnode->op_type, "\0");
    strcpy (pnode->status, "\0");
    return (OK_PARSER);
}

/***************************************************************************/
/* 04/25/01                                                                */
/* FUNCTION INIT_LINK_MAIN: INITIALIZE LINK VALUES TO BE USED IN THE MAIN  */
/***************************************************************************/
int
init_link_main (struct link_block *plink)
{
    strcpy (plink->name, "\0");
    strcpy (plink->from, "\0");
    strcpy (plink->to, "\0");
    plink->ID = -1;
    plink->distance = plink->ber = -1.0;
    plink->delay = plink->jitter = -1.0;
    plink->bw = plink->mfp = -1.0;
    plink->mtbf = plink->mttr = -1.0;
    plink->fiber_ratio = 0;
    plink->fiber_array = NULL;
    return (OK_PARSER);
}


/***************************************************************************/
/* 04/25/01                                                                */
/* FUNCTION FREE_LINK: INITIALIZE LINK VALUES                              */
/***************************************************************************/
int
free_link (struct link_block *plink)
{
    strcpy (plink->name, "\0");
    strcpy (plink->from, "\0");
    strcpy (plink->to, "\0");
    plink->ID = -1;
    plink->distance = plink->ber = -1.0;
    plink->delay = plink->jitter = -1.0;
    plink->bw = plink->mfp = -1.0;
    plink->mtbf = plink->mttr = -1.0;
    for (int i = 0; i < plink->fiber_ratio; i++)
    {
        free_lambda_array (plink->fiber_array[i].lambda_array);
        free_amply_array (plink->fiber_array[i].amply_array);
        plink->fiber_array[i].lambda_array = NULL;
        plink->fiber_array[i].amply_array = NULL;
    }
    plink->fiber_ratio = 0;
    plink->fiber_array = NULL;
    return (OK_PARSER);
}


/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTION GET_DECISION                                                    */
/****************************************************************************/
int
get_decision ()
{
    char str[STRING_LENGTH];
    int value = -1;
    while (1)
    {
        while (value == -1)
        {
            fprintf (stderr, "0: EXIT PROGRAM\n");
            fprintf (stderr, "1: CONTINUE PROGRAM\n");
            fprintf (stderr, "2: SKIP BLOCK\n");
            fgets (str, STRING_LENGTH + 1, stdin);
            if (sscanf (str, "%d", &value) == EOF)
                value = -1;
            switch (value)
            {
            case 0:
                return (STOP_PARSER);
            case 1:
                return (OK_PARSER);
            case 2:
                return (STOP_PARSER);
            default:
                value = -1;
            }
        }
    }
}

/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTION ERR_INPUT MANAGES ERRORS FROM PARSING THE INPUT FILE            */
/****************************************************************************/
int
err_input (int code, int line)
{
    int return_value;
    switch (code)
    {
    case ERR_SCAN:
    {
        fprintf (stderr, "SSCANF ARGUMENT FORMAT ERROR: INPUT LINE %d\n", line);
        return_value = get_decision ();
        break;
    }
    case NO_FIELDS:
    {
        fprintf (stderr, "NO VALID FIELDS FOUND: LINE %d\n", line);
        return_value = get_decision ();
        break;
    }
    case ERR_POS:
    {
        fprintf (stderr, "PARSING ERROR ON THE POSITION VALUE: INPUT LINE %d\n",
                 line);
        return_value = get_decision ();
        break;
    }
    case ERR_ID:
    {
        fprintf (stderr, "PARSING ERROR ON THE ID VALUE: INPUT LINE %d\n", line);
        return_value = get_decision ();
        break;
    }
    case ERR_TRX_BITRATE:
    {
        fprintf (stderr, "PARSING ERROR ON THE TRX_BITRATE VALUE: INPUT LINE %d\n",
                 line);
        return_value = get_decision ();
        break;
    }
    case ERR_SENS:
    {
        fprintf (stderr, "PARSING ERROR ON THE REC_SENSITIVITY VALUE: INPUT LINE %d\n",
                 line);
        return_value = get_decision ();
        break;
    }
    case ERR_BER:
    {
        fprintf (stderr, "PARSING ERROR ON THE BER VALUE: INPUT LINE %d\n",
                 line);
        return_value = get_decision ();
        break;
    }
    case ERR_DELAY:
    {
        fprintf (stderr, "PARSING ERROR ON THE DELAY VALUE: INPUT LINE %d\n",
                 line);
        return_value = get_decision ();
        break;
    }
    case ERR_JITTER:
    {
        fprintf (stderr, "PARSING ERROR ON THE JITTER VALUE: INPUT LINE %d\n",
                 line);
        return_value = get_decision ();
        break;
    }
    case ERR_BW:
    {
        fprintf (stderr, "PARSING ERROR ON THE BANDWIDTH VALUE: INPUT LINE %d\n",
                 line);
        return_value = get_decision ();
        break;
    }
    case ERR_OP_TYPE:
    {
        fprintf (stderr, "PARSING ERROR ON THE OP_TYPE VALUE: INPUT LINE %d\n",
                 line);
        return_value = get_decision ();
        break;
    }
    case ERR_STATUS:
    {
        fprintf (stderr, "PARSING ERROR ON THE STATUS VALUE: INPUT LINE %d\n",
                 line);
        return_value = get_decision ();
        break;
    }
    case ERR_MTBF:
    {
        fprintf (stderr, "PARSING ERROR ON THE MTBF VALUE: INPUT LINE %d\n",
                 line);
        return_value = get_decision ();
        break;
    }
    case ERR_MTTR:
    {
        fprintf (stderr, "PARSING ERROR ON THE MTTR VALUE: INPUT LINE %d\n",
                 line);
        return_value = get_decision ();
        break;
    }
    case ERR_DISTANCE:
    {
        fprintf (stderr, "PARSING ERROR ON THE DISTANCE VALUE: INPUT LINE %d\n",
                 line);
        return_value = get_decision ();
        break;
    }
    case ERR_MFP:
    {
        fprintf (stderr, "PARSING ERROR ON THE MFP VALUE: INPUT LINE %d\n",
                 line);
        return_value = get_decision ();
        break;
    }
    case ERR_FIBER_RATIO:
    {
        fprintf (stderr, "PARSING ERROR ON THE FIBER RATIO VALUE: INPUT LINE %d\n",
                 line);
        return_value = get_decision ();
        break;
    }
    case ERR_FIBER_LIST:
    {
        fprintf (stderr, "PARSING ERROR ON THE FIBER LIST: INPUT LINE %d\n",
                 line);
        return_value = get_decision ();
        break;
    }
    case ERR_AMPLIFIER_LIST:
    {
        fprintf (stderr, "PARSING ERROR READING AMPLIFIER LIST: INPUT LINE %d\n",
                 line);
        return_value = get_decision ();
        break;
    }
    case ERR_LAMBDA_LIST:
    {
        fprintf (stderr, "PARSING ERROR READING LAMBDA LIST: INPUT LINE %d\n",
                 line);
        return_value = get_decision ();
        break;
    }
    case ERR_SOURCE:
    {
        fprintf (stderr, "PARSING ERROR READING SOURCE: INPUT LINE %d\n",
                 line);
        return_value = get_decision ();
        break;
    }
    case ERR_DESTINATION:
    {
        fprintf (stderr, "PARSING ERROR READING DESTINATION: INPUT LINE %d\n",
                 line);
        return_value = get_decision ();
        break;
    }
    case ERR_DEMAND:
    {
        fprintf (stderr, "PARSING ERROR READING DEMAND AMOUNT: INPUT LINE %d\n",
                 line);
        return_value = get_decision ();
        break;
    }
    case ERR_DEMAND_TYPE:
    {
        fprintf (stderr, "PARSING ERROR READING DEMAND TYPE: INPUT LINE %d\n",
                 line);
        return_value = get_decision ();
        break;
    }
    case ERR_CLASS:
    {
        fprintf (stderr, "PARSING ERROR READING DEMAND CLASS: INPUT LINE %d\n",
                 line);
        return_value = get_decision ();
        break;
    }


    default:
    {
        fprintf (stderr, "UNRECOGNIZED ERROR CODE %d: INPUT LINE %d\n", code, line);
        return_value = get_decision ();
    }

    }
    if (return_value == STOP_PARSER)
        exit (STOP_PARSER);
    else
        return (OK_PARSER);
}



/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTIONS IS_VALID_XPOS RETURNS OK_PARSER IF XOPSITION IS A VALID RANGE  */
/****************************************************************************/
int
is_valid_xpos (float xpos)
{
    if (xpos != 0.000001)
        return OK_PARSER;
    else
    {
        fprintf (stderr, "Position = %f\n", xpos);
        fprintf (stderr, "If xpos==0.000001 -> check function is_valid_xpos\n");
        fprintf (stderr, "INVALID NODE POSITION VALUE: %f\n", xpos);
        return (NOT_OK_PARSER);
    }
}

/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTIONS IS_VALID_nodeID RETURNS OK IF XOPSITION IS A VALID RANGE       */
/****************************************************************************/
int
is_valid_nodeID (int ID)
{
    if (ID >= 0)
        return OK_PARSER;
    else
    {
        fprintf (stderr, "INVALID NODE ID VALUE: %d\n", ID);
        return (NOT_OK_PARSER);
    }
}

/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTIONS IS_VALID_BER RETURNS OK IF BER IS IN A VALID RANGE             */
/****************************************************************************/
int
is_valid_ber (float ber)
{
    if (ber >= 0)
        return OK_PARSER;
    else
    {
        fprintf (stderr, "INVALID BIT ERROR RATE VALUE: %f\n", ber);
        return (NOT_OK_PARSER);
    }
}

/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTIONS IS_VALID_REC_SENSITIVITY RETURNS OK_PARSER IF REC_SENSITIVITY  */
/* IS A VALID RANGE                                                         */
/****************************************************************************/
int
is_valid_rec_sensitivity (float rec_sensitivity)
{
    if (rec_sensitivity >= 0)
        return OK_PARSER;
    else
    {
        fprintf (stderr, "INVALID RECEIVER SENSITIVITY VALUE: %f\n", rec_sensitivity);
        return (NOT_OK_PARSER);
    }
}

/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTIONS IS_VALID_TRX_BITRATE RETURNS OK_PARSER IF TRX_BITRATE          */
/* IS A VALID RANGE                                                         */
/****************************************************************************/
int
is_valid_trx_bitrate (float trx_bitrate)
{
    if (trx_bitrate >= 0)
        return OK_PARSER;
    else
    {
        fprintf (stderr, "INVALID TRANSITTER BIT RATE VALUE: %f\n", trx_bitrate);
        return (NOT_OK_PARSER);
    }
}

/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTIONS IS_VALID_DELAY RETURNS OK_PARSER IF DELAY                      */
/* IS A VALID RANGE                                                         */
/****************************************************************************/
int
is_valid_delay (float delay)
{
    if (delay >= 0)
        return OK_PARSER;
    else
    {
        fprintf (stderr, "INVALID DELAY VALUE: %f\n", delay);
        return (NOT_OK_PARSER);
    }
}

/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTIONS IS_VALID_JITTER RETURNS OK_PARSER IF JITTER                    */
/* IS A VALID RANGE                                                         */
/****************************************************************************/
int
is_valid_jitter (float jitter)
{
    if (jitter >= 0)
        return OK_PARSER;
    else
    {
        fprintf (stderr, "INVALID JITTER VALUE: %f\n", jitter);
        return (NOT_OK_PARSER);
    }
}

/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTIONS IS_VALID_BW RETURNS OK_PARSER IF BANDWIDTH                     */
/* IS A VALID RANGE                                                         */
/****************************************************************************/
int
is_valid_bw (float bw)
{
    if (bw >= 0)
        return OK_PARSER;
    else
    {
        fprintf (stderr, "INVALID BANDWIDTH VALUE: %f\n", bw);
        return (NOT_OK_PARSER);
    }
}

/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTIONS IS_VALID_OP_TYPE RETURNS OK_PARSER IF OP_TYPE                  */
/* IS A VALID                                                               */
/****************************************************************************/
int
is_valid_op_type (char *op_type)
{
    if (strncmp (op_type, "NORMAL", 6) == 0)
        return OK_PARSER;
    else
    {
        fprintf (stderr, "INVALID OP_TYPE VALUE: %s\n", op_type);
        return (NOT_OK_PARSER);
    }
}

/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTIONS IS_VALID_STATUS RETURNS OK_PARSER IF STATUS                    */
/* IS A VALID                                                               */
/****************************************************************************/
int
is_valid_status (char *status)
{
    if (strncmp (status, "WORKING", 7) == 0)
        return OK_PARSER;
    else
    {
        fprintf (stderr, "INVALID STATUS VALUE: %s\n", status);
        return (NOT_OK_PARSER);
    }
}

/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTIONS IS_VALID_MTBF RETURNS OK_PARSER IF MTBF                        */
/* IS A VALID RANGE                                                         */
/****************************************************************************/
int
is_valid_mtbf (float mtbf)
{
    if (mtbf >= 0)
        return OK_PARSER;
    else
    {
        fprintf (stderr, "INVALID MTBF VALUE: %f\n", mtbf);
        return (NOT_OK_PARSER);
    }
}

/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTIONS IS_VALID_MTTR RETURNS OK_PARSER IF MTTR                        */
/* IS A VALID RANGE                                                         */
/****************************************************************************/
int
is_valid_mttr (float mttr)
{
    if (mttr >= 0)
        return OK_PARSER;
    else
    {
        fprintf (stderr, "INVALID MTTR VALUE: %f\n", mttr);
        return (NOT_OK_PARSER);
    }
}

/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTIONS IS_VALID_LINKID RETURNS OK_PARSER IF LINKID                    */
/* IS A VALID RANGE                                                         */
/****************************************************************************/
int
is_valid_linkID (int linkID)
{
    if (linkID >= 0)
        return OK_PARSER;
    else
    {
        fprintf (stderr, "INVALID LINK ID VALUE: %d\n", linkID);
        return (NOT_OK_PARSER);
    }
}


/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTIONS IS_VALID_LINK_DISTANCE RETURNS OK_PARSER IF DISTANCE           */
/* IS A VALID RANGE                                                         */
/****************************************************************************/
int
is_valid_link_distance (float distance)
{
    if (distance >= 0)
        return OK_PARSER;
    else
    {
        fprintf (stderr, "INVALID DISTANCE VALUE: %f\n", distance);
        return (NOT_OK_PARSER);
    }
}
/* -------------------------  added by Marco Ghizzi --------------------*/

/****************************************************************************/
/* 11/20/02                                                                 */
/* FUNCTIONS IS_VALID_LINK_MAXBYTE RETURNS OK_PARSER IF MAXBYTE             */
/* IS A VALID RANGE                                                         */
/****************************************************************************/

int
is_valid_link_maxbyte (float maxbyte)
{
    if (maxbyte >= 0)
        return OK_PARSER;
    else
    {
        fprintf (stderr, "INVALID MAXBYTE VALUE: %f\n", maxbyte);
        return (NOT_OK_PARSER);
    }
}

/*********************************************************************************/
/* 11/20/02                                                                      */
/* FUNCTIONS IS_VALID_LINK_NUM_CHANNEL (lambda) RETURNS OK_PARSER IF NUM_CHANNEL */
/* IS A VALID RANGE                                                              */
/*********************************************************************************/

int
is_valid_link_num_channel (float num_channel)
{
    if (num_channel >= 0)
        return OK_PARSER;
    else
    {
        fprintf (stderr, "INVALID NUM_CHANNEL VALUE: %f\n", num_channel);
        return (NOT_OK_PARSER);
    }
}

/*********************************************************************************/
/* 12/03/02                                                                      */
/* FUNCTIONS IS_VALID_LINK_BITRATE RETURNS OK_PARSER IF BITRATE */
/* IS A VALID RANGE                                                              */
/*********************************************************************************/

int
is_valid_link_bitrate (float bitrate)
{
    if (bitrate >= 0)
        return OK_PARSER;
    else
    {
        fprintf (stderr, "INVALID NUM_CHANNEL VALUE: %f\n", bitrate);
        return (NOT_OK_PARSER);
    }
}

/* -------------------------  end ------------------------------------------*/

/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTIONS IS_VALID_LINK_BER RETURNS OK_PARSER IF BER                     */
/* IS A VALID RANGE                                                         */
/****************************************************************************/
int
is_valid_link_ber (float ber)
{
    if (ber >= 0)
        return OK_PARSER;
    else
    {
        fprintf (stderr, "INVALID LINK BER VALUE: %f\n", ber);
        return (NOT_OK_PARSER);
    }
}

/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTIONS IS_VALID_LINK_DELAY RETURNS OK_PARSER IF DELAY                 */
/* IS A VALID RANGE                                                         */
/****************************************************************************/
int
is_valid_link_delay (float delay)
{
    if (delay >= 0)
        return OK_PARSER;
    else
    {
        fprintf (stderr, "INVALID LINK DELAY VALUE: %f\n", delay);
        return (NOT_OK_PARSER);
    }
}

/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTIONS IS_VALID_LINK_JITTER RETURNS OK_PARSER IF JITTER               */
/* IS A VALID RANGE                                                         */
/****************************************************************************/
int
is_valid_link_jitter (float jitter)
{
    if (jitter >= 0)
        return OK_PARSER;
    else
    {
        fprintf (stderr, "INVALID LINK JITTER VALUE: %f\n", jitter);
        return (NOT_OK_PARSER);
    }
}

/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTIONS IS_VALID_LINK_BANDWIDTH RETURNS OK_PARSER IF BANDWIDTH         */
/* IS A VALID RANGE                                                         */
/****************************************************************************/
int
is_valid_link_bandwidth (float bw)
{
    if (bw >= 0)
        return OK_PARSER;
    else
    {
        fprintf (stderr, "INVALID LINK BANDWIDTH VALUE: %f\n", bw);
        return (NOT_OK_PARSER);
    }
}

/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTIONS IS_VALID_LINK_MFP RETURNS OK_PARSER IF MFP                     */
/* IS A VALID RANGE                                                         */
/****************************************************************************/
int
is_valid_link_mfp (float mfp)
{
    if (mfp >= 0)
        return OK_PARSER;
    else
    {
        fprintf (stderr, "INVALID LINK_MFP VALUE: %f\n", mfp);
        return (NOT_OK_PARSER);
    }
}

/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTIONS IS_VALID_LINK_MTBF RETURNS OK_PARSER IF MTBF                   */
/* IS A VALID RANGE                                                         */
/****************************************************************************/
int
is_valid_link_mtbf (float mtbf)
{
    if (mtbf >= 0)
        return OK_PARSER;
    else
    {
        fprintf (stderr, "INVALID LINK MTBF VALUE: %f\n", mtbf);
        return (NOT_OK_PARSER);
    }
}

/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTIONS IS_VALID_LINK_MTTR RETURNS OK_PARSER IF MTTR                   */
/* IS A VALID RANGE                                                         */
/****************************************************************************/
int
is_valid_link_mttr (float mttr)
{
    if (mttr >= 0)
        return OK_PARSER;
    else
    {
        fprintf (stderr, "INVALID MTTR VALUE: %f\n", mttr);
        return (NOT_OK_PARSER);
    }
}

/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTIONS IS_VALID_LINK_FIBER_RATIO RETURNS OK_PARSER IF FIBER_RATIO     */
/* IS A VALID RANGE                                                         */
/****************************************************************************/
int
is_valid_link_fiber_ratio (int fiber_ratio)
{
    if (fiber_ratio >= 0)
        return OK_PARSER;
    else
    {
        fprintf (stderr, "INVALID FIBER RATIO VALUE: %d\n", fiber_ratio);
        return (NOT_OK_PARSER);
    }
}

/****************************************************************************/
/* 04/18/01                                                                 */
/* FUNCTIONS IS_VALID_FIBER_LIST_ID RETURNS OK_PARSER IF ID                 */
/* IS IN A VALID RANGE                                                      */
/****************************************************************************/
int
is_valid_fiber_list_ID (int ID)
{
    if (ID >= 0)
        return (OK_PARSER);
    else
    {
        fprintf (stderr, "INVALID FIBER ID: %d\n", ID);
        return (NOT_OK_PARSER);
    }
}

/****************************************************************************/
/* 04/18/01                                                                 */
/* FUNCTIONS IS_VALID_LIST_OP_TYPE RETURNS OK_PARSER IF OP_TYPE             */
/* IS IN A VALID RANGE                                                      */
/****************************************************************************/
int
is_valid_fiber_list_op_type (char *op_type)
{
    if (strncmp (op_type, "NORMAL", 6) == 0)
        return (OK_PARSER);
    if (strncmp (op_type, "normal", 6) == 0)
        return (OK_PARSER);
    fprintf (stderr, "INVALID FIBER LIST OP_TYPE %s\n", op_type);
    return (NOT_OK_PARSER);
}

/****************************************************************************/
/* 04/18/01                                                                 */
/* FUNCTIONS IS_VALID_FIBER_LIST_STATUS RETURNS OK_PARSER IF STATUS         */
/* IS IN A VALID RANGE                                                      */
/****************************************************************************/
int
is_valid_fiber_list_status (char *status)
{
    if (strncmp (status, "WORKING", 7) == 0)
        return (OK_PARSER);
    if (strncmp (status, "working", 7) == 0)
        return (OK_PARSER);
    fprintf (stderr, "INVALID FIBER LIST STATUS: %s\n", status);
    return (NOT_OK_PARSER);
}

/****************************************************************************/
/* 04/18/01                                                                 */
/* FUNCTIONS IS_VALID_FIBER_LIST_LAMBDA_RATIO RETURNS OK_PARSER IF          */
/* LAMBDA_RATIO                                                             */
/* IS IN A VALID RANGE                                                      */
/****************************************************************************/
int
is_valid_fiber_list_lambda_ratio (int lambda_ratio)
{
    if (lambda_ratio > 0)
        return (OK_PARSER);
    fprintf (stderr, "INVALID VALUE FOR LAMBDA_RATIO: %d\n", lambda_ratio);
    return (NOT_OK_PARSER);
}


/****************************************************************************/
/* 04/26/01                                                                 */
/* FUNCTION IS_VALID_LAMBDA_STATUS                                          */
/****************************************************************************/
int
is_valid_lambda_status (char *lambda_status)
{
    if (strncmp (lambda_status, "WORKING", 7) == 0)
        return (OK_PARSER);
    if (strncmp (lambda_status, "working", 7) == 0)
        return (OK_PARSER);
    fprintf (stderr, "INVALID VALUE FOR LAMBDA STATUS %s\n", lambda_status);
    return (NOT_OK_PARSER);
}

/****************************************************************************/
/* 04/26/01                                                                 */
/* FUNCTION IS_VALID_LAMBDA_OP_TYPE                                         */
/****************************************************************************/
int
is_valid_lambda_op_type (char *lambda_status)
{
    if (strncmp (lambda_status, "NORMAL", 6) == 0)
        return (OK_PARSER);
    if (strncmp (lambda_status, "normal", 6) == 0)
        return (OK_PARSER);
    fprintf (stderr, "INVALID VALUE FOR LAMBDA STATUS %s\n", lambda_status);
    return (NOT_OK_PARSER);
}

/****************************************************************************/
/* 04/26/01                                                                 */
/* FUNCTION IS_VALID_LAMBDA_ID                                              */
/****************************************************************************/
int
is_valid_lambda_ID (int ID)
{
    if (ID >= 0)
        return (OK_PARSER);
    fprintf (stderr, "INVALID VALUE FOR LAMBDA ID %d\n", ID);
    return (NOT_OK_PARSER);
}

/****************************************************************************/
/* 04/18/01                                                                 */
/* FUNCTION IS_VALID_FIBER_AMPLIFIER_RATIO RETURNS OK_PARSER IF THE         */
/* AMPLIFIER RATIO IS IN A VALID RANGE                                      */
/****************************************************************************/
int
is_valid_fiber_amplifier_ratio (int ratio)
{
    if (ratio > 0)
        return (OK_PARSER);
    fprintf (stderr, "INVALID VALUE FOR AMPLIFIER RATIO: %d\n", ratio);
    return (NOT_OK_PARSER);
}
/****************************************************************************/
/* 04/26/01                                                                 */
/* FUNCTION IS VALID AMPLY POS RETURN OK_PARSER IF 0 < POS < DISTANCE       */
/****************************************************************************/
int
is_valid_amply_pos (float pos, float distance)
{
    if (pos >= 0)
        if (pos <= distance)
            return (OK_PARSER);
    fprintf (stderr, "INVALID VALUE FOR AMPLIFIER POSITION %f\n", pos);
    return (NOT_OK_PARSER);
}



/*************************************************************************/
/* 04/18/01                                                              */
/* FUNCTION ALLOCATE_FIBER_LIST_ARRAY ALLOCATES THE ARRAY FOR THE        */
/* FIBER ARRAY                                                           */
/*************************************************************************/
int
allocate_fiber_list_array (struct link_block *p_link)
{
    if (((p_link->fiber_array) = (struct fiber_list *)
                                 malloc (p_link->fiber_ratio * sizeof (struct fiber_list))) == NULL)
        exit (ERR_MEM_PARSER);
    else
        return (OK_PARSER);
}


/*************************************************************************/
/* 04/18/01                                                              */
/* FUNCTION ALLOCATE_AMPLIFIER_ARRAY ALLOCATES THE ARRAY NECESSARY TO    */
/* CONTAIN THE INFORMATION ON THE AMPLY_ARRAY ARRAY                      */
/*************************************************************************/
int
allocate_amplifier_array (struct fiber_list *elem)
{
    if (((elem->amply_array) = (struct amplifier_elem *)
                               malloc (elem->amplifier_ratio * sizeof (struct amplifier_elem))) == NULL)
        exit (ERR_MEM_PARSER);
    else
        return (OK_PARSER);
}

/*************************************************************************/
/* 04/18/01                                                              */
/* FUNCTION ALLOCATE_LAMBDA_LIST ALLOCATES THE ARRAY NECESSARY TO        */
/* CONTAIN THE INFORMATION ON THE LAMBDA_LIST LIST                       */
/*************************************************************************/
int
allocate_lambda_list_array (struct fiber_list *elem)
{
    if (((elem->lambda_array) = (struct lambda_elem *)
                                malloc (elem->lambda_ratio * sizeof (struct lambda_elem))) == NULL)
        exit (ERR_MEM_PARSER);
    else
        return (OK_PARSER);
}

/************************************************************************/
/* 04/18/01                                                             */
/* FUNCTION READ_LAMBDA_LIST_BLOCK READS A LAMBDA BLOCK IN THE FIBER    */
/* LIST                                                                 */
/************************************************************************/
int
read_lambda_list_block (FILE * input_stream, struct lambda_elem *elem,
                        int n_lambda, int *input_line_number)
{
    char *temp;
    int i = 0;
    int return_value;
    char str[STRING_LENGTH], input_string[STRING_LENGTH];
    strcpy (str, "\0");
    while (str[0] != '}')
    {
        fgets (input_string, STRING_LENGTH + 1, input_stream);
        (*input_line_number)++;
        return_value = sscanf (input_string, "%s", str);
        if ((return_value == 0) || (return_value == EOF))
            strcpy (str, input_string);
        if (str[0] == '}')
            break;
        if (is_comment (str) != OK_PARSER)
        {
            return_value = sscanf (input_string, "%s %s %s", elem[i].lambda,
                                   elem[i].op_type, elem[i].status);
            if ((return_value < 3) || (return_value == EOF))
                return (NOT_OK_PARSER);
            temp = strstr (elem[i].lambda, "_");
            temp++;
            return_value = sscanf (temp, "%d", &(elem[i].lambdaID));
            if ((return_value == 0) || (return_value == EOF))
                return (NOT_OK_PARSER);
            i++;
        }
    }
    if (i != n_lambda)
    {
        fprintf (stderr, "NUMBER OF LAMBDA's: %d\nLAMBDA RATIO: %d\n",
                 i, n_lambda);
        return (NOT_OK_PARSER);
    }
    return (OK_PARSER);
}

/************************************************************************/
/* 04/18/01                                                             */
/* FUNCTION READ_AMPLIFIER_BLOCK READS AN AMPLY BLOCK IN THE FIBER      */
/* LIST                                                                 */
/************************************************************************/
int
read_amplifier_block (FILE * input_stream, struct amplifier_elem *elem,
                      int n_amply, int *input_line_number)
{
    int i = 0;
    int return_value;
    char str[STRING_LENGTH], input_string[STRING_LENGTH];
    strcpy (str, "\0");
    while (str[0] != '}')
    {
        fgets (input_string, STRING_LENGTH + 1, input_stream);
        (*input_line_number)++;
        return_value = sscanf (input_string, "%s", str);
        if ((return_value == 0) || (return_value == EOF))
            strcpy (str, input_string);
        if (str[0] == '}')
            break;
        if (is_comment (str) != OK_PARSER)
        {
            return_value = sscanf (input_string, "%s %s %f", elem[i].amplifier_type,
                                   elem[i].amplifier_name, &(elem[i].position));
            if ((return_value < 3) || (return_value == EOF))
                return (NOT_OK_PARSER);
            i++;
        }
    }
    if (i != n_amply)
    {
        fprintf (stderr, "NUMBER OF AMPLIFIERS: %d\nAMPLIFIER RATIO: %d\n",
                 i, n_amply);
        return (NOT_OK_PARSER);
    }
    return (OK_PARSER);
}

/***************************************************************************/
/* 04/26/01                                                                */
/* FUNCTION IS_FIBER_TYPE RETURNS OK_PARSER IF STR IS A VALID FIBER TYPE   */
/***************************************************************************/
int
is_valid_fiber_type (char *str)
{
    if (strncmp (str, "MMF", 3) == 0)
        return (OK_PARSER);
    if (strncmp (str, "mmf", 3) == 0)
        return (OK_PARSER);
    if (strncmp (str, "SMF", 3) == 0)
        return (OK_PARSER);
    if (strncmp (str, "smf", 3) == 0)
        return (OK_PARSER);
    if (strncmp (str, "ZDF", 3) == 0)
        return (OK_PARSER);
    if (strncmp (str, "zdf", 3) == 0)
        return (OK_PARSER);
    if (strncmp (str, "NZDF", 4) == 0)
        return (OK_PARSER);
    if (strncmp (str, "nzdf", 4) == 0)
        return (OK_PARSER);
    return (NOT_OK_PARSER);
}

/***************************************************************************/
/* 03/23/01                                                                */
/* FUNCTION READ_FIBER_LIST_BLOCK: READ THE FIBER_LIST INTO THE LINK BLOCK */
/***************************************************************************/
int
read_fiber_list_block (struct fiber_list *fiber_array, int i, int
                       *input_line_number, FILE * input_stream)
{
    int sscanf_value;
    int line = 0;
    char str[STRING_LENGTH], input_string[STRING_LENGTH];
    strcpy (str, "QWERTY");
    while (str[0] != '}')
    {
        //if (is_fiber_list_element(str)==OK_PARSER)
        {
            /* BLOCK IS A FIBER LIST BLOCK */
            fgets (input_string, STRING_LENGTH + 1, input_stream);
            (*input_line_number)++;
            sscanf_value = sscanf (input_string, "%s", str);
            if ((sscanf_value == 0) || (sscanf_value == EOF))
                strcpy (str, input_string);
            if (is_comment (str) != YES_PARSER)
            {

                /**********************************************************************/
                /**********************************************************************/
                /**********************************************************************/
                /* IN CASE NEW ELEMENTS ARE ADDED THEY MUST BE READ IN THIS IF        */
                /**********************************************************************/
                /**********************************************************************/
                /**********************************************************************/
                if (line == 0)
                    if (str[0] == '}')
                        return (END_OF_FIBER_LIST);
                line++;
                if (is_valid_fiber_type (str) == OK_PARSER)
                {
                    sscanf_value = sscanf (input_string, "%s %s", (fiber_array[i].type),
                                           (fiber_array[i].name));
                    if ((sscanf_value < 2) || (sscanf_value == EOF))
                        err_input (ERR_SCAN, (*input_line_number));

                }

                if ((strcmp (str, "ID") == 0) || (strcmp (str, "id") == 0))
                {
                    sscanf_value = sscanf (input_string, "%s %d", str, &(fiber_array[i].ID));
                    if ((sscanf_value < 2) || (sscanf_value == EOF))
                        err_input (ERR_SCAN, (*input_line_number));
                    if (is_valid_fiber_list_ID (fiber_array[i].ID) != OK_PARSER)
                        err_input (ERR_FIBER_ID, (*input_line_number));
                }

                if ((strcmp (str, "OP_TYPE") == 0) || (strcmp (str, "op_type") == 0))
                {
                    sscanf_value = sscanf (input_string, "%s %s", str, (fiber_array[i].op_type));
                    if ((sscanf_value < 2) || (sscanf_value == EOF))
                        err_input (ERR_SCAN, (*input_line_number));
                    if (is_valid_fiber_list_op_type (fiber_array[i].op_type) != OK_PARSER)
                        err_input (ERR_FIBER_OP_TYPE, (*input_line_number));
                }

                if ((strcmp (str, "STATUS") == 0) || (strcmp (str, "status") == 0))
                {
                    sscanf_value = sscanf (input_string, "%s %s", str, (fiber_array[i].status));
                    if ((sscanf_value < 2) || (sscanf_value == EOF))
                        err_input (ERR_SCAN, (*input_line_number));
                    if (is_valid_fiber_list_status (fiber_array[i].status) != OK_PARSER)
                        err_input (ERR_FIBER_STATUS, (*input_line_number));
                }

                if ((strcmp (str, "LAMBDA_RATIO") == 0) || (strcmp (str, "lambda_ratio") == 0))
                {
                    sscanf_value = sscanf (input_string, "%s %d", str, &(fiber_array[i].
                                           lambda_ratio));
                    if ((sscanf_value < 2) || (sscanf_value == EOF))
                        err_input (ERR_SCAN, (*input_line_number));
                    if (is_valid_fiber_list_lambda_ratio (fiber_array[i].lambda_ratio)
                            != OK_PARSER)
                        err_input (ERR_FIBER_LAMBDA_RATIO, (*input_line_number));
                }

                if ((strcmp (str, "LAMBDA_LIST") == 0) || (strcmp (str, "lambda_list") == 0))
                {
                    if (is_valid_fiber_list_lambda_ratio (fiber_array[i].lambda_ratio)
                            != OK_PARSER)
                        err_input (ERR_FIBER_LAMBDA_RATIO, (*input_line_number));
                    if (allocate_lambda_list_array (&(fiber_array[i])) != OK_PARSER)
                        exit (ERR_MEM_PARSER);
                    /* READ FIBER LIST BLOCK */
                    if (read_lambda_list_block (input_stream, fiber_array[i].lambda_array,
                                                fiber_array[i].lambda_ratio, input_line_number) != OK_PARSER)
                        err_input (ERR_LAMBDA_LIST, (*input_line_number));

                }

                if ((strcmp (str, "AMPLIFIER_RATIO") == 0) ||
                        (strcmp (str, "amplifier_ratio") == 0))
                {
                    sscanf_value = sscanf (input_string, "%s %d",
                                           str, &(fiber_array[i].amplifier_ratio));
                    if ((sscanf_value < 2) || (sscanf_value == EOF))
                        err_input (ERR_SCAN, (*input_line_number));
                    if (is_valid_fiber_amplifier_ratio (fiber_array[i].amplifier_ratio)
                            != OK_PARSER)
                        err_input (ERR_AMPLIFIER_RATIO, (*input_line_number));
                }

                if ((strcmp (str, "AMPLIFIER_LIST") == 0) || (strcmp (str, "amplifier_list") == 0))
                {
                    if (is_valid_fiber_amplifier_ratio (fiber_array[i].amplifier_ratio)
                            != OK_PARSER)
                        err_input (ERR_AMPLIFIER_RATIO, (*input_line_number));
                    if (allocate_amplifier_array (&(fiber_array[i])) != OK_PARSER)
                        exit (ERR_MEM_PARSER);

                    /* READ AMPLIFIER LIST BLOCK */
                    if (read_amplifier_block (input_stream, fiber_array[i].amply_array,
                                              fiber_array[i].amplifier_ratio, input_line_number) != OK_PARSER)
                        err_input (ERR_AMPLIFIER_LIST, (*input_line_number));


                }

            }
        }
    }
    return (OK_PARSER);
}

/****************************************************************************/
/* 03/03/10                                                                 */
/* FUNCTION READ_DOMAIN_BLOCK: READS A BLOCK OF DATA AND PUTS IT IN THE     */
/* APPROPRIATE STRUCTURE                                                    */
/* By Cialo & Wamba @ SSSUP						    */
/****************************************************************************/
int read_domain_block (struct domain_block *domain, FILE * input_stream, int *input_line_number) {
    int sscanf_value;
    int line_read = 0;
    int read_fields = 0;
    char str[STRING_LENGTH], input_string[STRING_LENGTH];
    char *token_string;

    strcpy (str, "QWERTY");
    init_domain (domain);

    while (str[0] != '}')
    {
        fgets (input_string, STRING_LENGTH + 1, input_stream);
        (*input_line_number)++;
        sscanf_value = sscanf (input_string, "%s", str);
        if ((sscanf_value == 0) || (sscanf_value == EOF))
            strcpy (str, input_string);
        if (line_read == 0)
            if (str[0] == '}')
                return (END_OF_DOMAIN_BLOCK);
        if (is_comment (str) != YES_PARSER)
            /* BLOCK IS A NETWORK_DOMAIN BLOCK */
        {
            line_read++;

            /**********************************************************************/
            /* IN CASE NEW ELEMENTS ARE ADDED THEY MUST BE READ IN THIS IF        */
            /**********************************************************************/

            if ((strcmp (str, "NETWORK_DOMAIN") == 0) || (strcmp (str, "network_domain") == 0))
                sscanf_value = sscanf (input_string, "%s %s", domain->type, domain->name);

            if ((sscanf_value < 2) || (sscanf_value == EOF))
                err_input (ERR_SCAN, *input_line_number);

            while (str[0] != '}') {
                fgets (input_string, STRING_LENGTH + 1, input_stream);
                (*input_line_number)++;

                sscanf_value = sscanf (input_string, "%s", str);
                if ((sscanf_value == 0) || (sscanf_value == EOF))
                    strcpy (str, input_string);

                if (line_read == 0)
                    if (str[0] == '}')
                        return (END_OF_DOMAIN_BLOCK);

                if (is_comment (str) != YES_PARSER) {
                    line_read++;

                    /**********************************************************************/
                    /* IN CASE NEW ELEMENTS ARE ADDED THEY MUST BE READ IN THIS IF        */
                    /**********************************************************************/

                    if (strncmp (str, "ID", 2) == 0) {
                        read_fields++;
                        sscanf_value = sscanf (input_string, "%s %d", str, &(domain->ID));
                        if ((sscanf_value < 2) || (sscanf_value == EOF))
                            err_input (ERR_SCAN, *input_line_number);
                    }

                    if (strncmp (str, "NODES", 5) == 0) {
                        read_fields++;
                        token_string = strtok (input_string," ;\n");
                        token_string = strtok (NULL, " ;\n");
                        int k=0;
                        while (token_string != NULL) {
                            domain->NodeIDs_List[k] = atoi(token_string);
                            token_string = strtok (NULL, "  ;\n");
                            k++;
                        }
                    }

                    if (strncmp (str, "PCE_NODE_ID", 11) == 0) {
                        read_fields++;
                        sscanf_value = sscanf (input_string, "%s %d", str, &(domain->PCE_ID));
                        if ((sscanf_value < 2) || (sscanf_value == EOF))
                            err_input (ERR_SCAN, *input_line_number);
                    }

                }
            }
        }
    }

    if (read_fields <= 0)
        err_input (NO_FIELDS, *input_line_number);
    return (OK_PARSER);
}


/****************************************************************************/
/* 03/23/01                                                                 */
/* FUNCTION READ_NODE_BLOCK: READS A BLOCK OF DATA AND PUTS IT IN THE       */
/* APPROPRIATE STRUCTURE                                                    */
/****************************************************************************/
int
read_node_block (struct node_block *node,
                 FILE * input_stream, int *input_line_number)
{
    int sscanf_value;
    int line_read = 0;
    int read_fields = 0;
    char str[STRING_LENGTH], input_string[STRING_LENGTH];
    strcpy (str, "QWERTY");
    init_node (node);
    while (str[0] != '}')
    {
        fgets (input_string, STRING_LENGTH + 1, input_stream);
        (*input_line_number)++;
        sscanf_value = sscanf (input_string, "%s", str);
        if ((sscanf_value == 0) || (sscanf_value == EOF))
            strcpy (str, input_string);
        if (line_read == 0)
            if (str[0] == '}')
                return (END_OF_NODE_BLOCK);
        if (is_comment (str) != YES_PARSER)
            /* BLOCK IS A OXC_CONVER BLOCK */
        {
            line_read++;

            /**********************************************************************/
            /**********************************************************************/
            /**********************************************************************/
            /* IN CASE NEW ELEMENTS ARE ADDED THEY MUST BE READ IN THIS IF        */
            /**********************************************************************/
            /**********************************************************************/
            /**********************************************************************/

            if ((strcmp (str, "OXC_CONVERT") == 0) || (strcmp (str, "oxc_convert") == 0))

                sscanf_value = sscanf (input_string, "%s %s", node->type, node->name);
            if ((sscanf_value < 2) || (sscanf_value == EOF))
                err_input (ERR_SCAN, *input_line_number);
            while (str[0] != '}')
            {
                fgets (input_string, STRING_LENGTH + 1, input_stream);
                (*input_line_number)++;
                sscanf_value = sscanf (input_string, "%s", str);
                if ((sscanf_value == 0) || (sscanf_value == EOF))
                    strcpy (str, input_string);
                if (line_read == 0)
                    if (str[0] == '}')
                        return (END_OF_NODE_BLOCK);
                if (is_comment (str) != YES_PARSER)
                    /* BLOCK IS A OXC_CONVER BLOCK */
                {
                    line_read++;



                    /**********************************************************************/
                    /**********************************************************************/
                    /**********************************************************************/
                    /* IN CASE NEW ELEMENTS ARE ADDED THEY MUST BE READ IN THIS IF        */
                    /**********************************************************************/
                    /**********************************************************************/
                    /**********************************************************************/


                    if ((strncmp (str, "XPOS", 4) == 0) || (strncmp (str, "xpos", 4) == 0))
                        /* READ XPOSITION */
                    {
                        read_fields++;
                        sscanf_value = sscanf (input_string, "%s %f", str, &(node->xpos));
                        if ((sscanf_value < 2) || (sscanf_value == EOF))
                            err_input (ERR_SCAN, *input_line_number);
                        if ((is_valid_xpos (node->xpos)) != OK_PARSER)
                            err_input (ERR_POS, *input_line_number);
                    }

                    if ((strncmp (str, "YPOS", 4) == 0) || (strncmp (str, "ypos", 4) == 0))
                        /* READ YPOSITION */
                    {
                        read_fields++;
                        sscanf_value = sscanf (input_string, "%s %f", str, &(node->ypos));
                        if ((sscanf_value < 2) || (sscanf_value == EOF))
                            err_input (ERR_SCAN, *input_line_number);
                        if ((is_valid_xpos (node->ypos)) != OK_PARSER)
                            err_input (ERR_POS, *input_line_number);
                    }

                    if ((strncmp (str, "ID", 2) == 0) || (strncmp (str, "id", 2) == 0))
                        /* READ NODE ID */
                    {
                        read_fields++;
                        sscanf_value = sscanf (input_string, "%s %d", str, &(node->ID));
                        if ((sscanf_value < 2) || (sscanf_value == EOF))
                            err_input (ERR_SCAN, *input_line_number);
                        if ((is_valid_nodeID (node->ID)) != OK_PARSER)
                            err_input (ERR_ID, *input_line_number);
                    }

                    if ((strncmp (str, "TRX_BITRATE", 11) == 0) ||
                            (strncmp (str, "trx_bitrate", 11) == 0))
                        /* READ TRX_BITRATE */
                    {
                        read_fields++;
                        sscanf_value = sscanf (input_string, "%s %f", str, &(node->trx_bitrate));
                        if ((sscanf_value < 2) || (sscanf_value == EOF))
                            err_input (ERR_SCAN, *input_line_number);
                        if ((is_valid_trx_bitrate (node->trx_bitrate)) != OK_PARSER)
                            err_input (ERR_TRX_BITRATE, *input_line_number);
                    }

                    if ((strncmp (str, "REC_SENSITIVITY", 15) == 0) ||
                            (strncmp (str, "rec_sensitivity", 15) == 0))
                        /* READ RECEIVER SENSITIVITY */
                    {
                        read_fields++;
                        sscanf_value = sscanf (input_string, "%s %f", str, &(node->rec_sensitivity));
                        if ((sscanf_value < 2) || (sscanf_value == EOF))
                            err_input (ERR_SCAN, *input_line_number);
                        if ((is_valid_rec_sensitivity (node->rec_sensitivity)) != OK_PARSER)
                            err_input (ERR_SENS, *input_line_number);
                    }

                    if ((strncmp (str, "BER", 3) == 0) ||
                            (strncmp (str, "ber", 3) == 0))
                        /* READ BER */
                    {
                        read_fields++;
                        sscanf_value = sscanf (input_string, "%s %f", str, &(node->ber));
                        if ((sscanf_value < 2) || (sscanf_value == EOF))
                            err_input (ERR_SCAN, *input_line_number);
                        if ((is_valid_ber (node->ber)) != OK_PARSER)
                            err_input (ERR_BER, *input_line_number);
                    }

                    if ((strncmp (str, "DELAY", 5) == 0) || (strncmp (str, "delay", 5) == 0))
                        /* READ DELAY */
                    {
                        read_fields++;
                        sscanf_value = sscanf (input_string, "%s %f", str, &(node->delay));
                        if ((sscanf_value < 2) || (sscanf_value == EOF))
                            err_input (ERR_SCAN, *input_line_number);
                        if ((is_valid_delay (node->delay)) != OK_PARSER)
                            err_input (ERR_DELAY, *input_line_number);
                    }

                    if ((strncmp (str, "JITTER", 6) == 0) || (strncmp (str, "jitter", 6) == 0))
                        /* READ JITTER */
                    {
                        read_fields++;
                        sscanf_value = sscanf (input_string, "%s %f", str, &(node->jitter));
                        if ((sscanf_value < 2) || (sscanf_value == EOF))
                            err_input (ERR_SCAN, *input_line_number);
                        if ((is_valid_jitter (node->jitter)) != OK_PARSER)
                            err_input (ERR_JITTER, *input_line_number);
                    }

                    if ((strncmp (str, "BW", 2) == 0) || (strncmp (str, "bw", 2) == 0))
                        /* READ NODE BANDWIDTH */
                    {
                        read_fields++;
                        sscanf_value = sscanf (input_string, "%s %f", str, &(node->bw));
                        if ((sscanf_value < 2) || (sscanf_value == EOF))
                            err_input (ERR_SCAN, *input_line_number);
                        if ((is_valid_bw (node->bw)) != OK_PARSER)
                            err_input (ERR_BW, *input_line_number);
                    }

                    if ((strncmp (str, "OP_TYPE", 7) == 0) || (strncmp (str, "op_type", 7) == 0))
                        /* READ OP_TYPE */
                    {
                        read_fields++;
                        sscanf_value = sscanf (input_string, "%s %s", str, (node->op_type));
                        if ((sscanf_value < 2) || (sscanf_value == EOF))
                            err_input (ERR_SCAN, *input_line_number);
                        if ((is_valid_op_type (node->op_type)) != OK_PARSER)
                            err_input (ERR_OP_TYPE, *input_line_number);
                    }

                    if ((strncmp (str, "STATUS", 7) == 0) || (strncmp (str, "status", 7) == 0))
                        /* READ STATUS */
                    {
                        read_fields++;
                        sscanf_value = sscanf (input_string, "%s %s", str, (node->status));
                        if ((sscanf_value < 2) || (sscanf_value == EOF))
                            err_input (ERR_SCAN, *input_line_number);
                        if ((is_valid_status (node->status)) != OK_PARSER)
                            err_input (ERR_STATUS, *input_line_number);
                    }

                    if ((strncmp (str, "MTBF", 4) == 0) || (strncmp (str, "mtbf", 4) == 0))
                        /* READ MTBF */
                    {
                        read_fields++;
                        sscanf_value = sscanf (input_string, "%s %f", str, &(node->mtbf));
                        if ((sscanf_value < 2) || (sscanf_value == EOF))
                            err_input (ERR_SCAN, *input_line_number);
                        if ((is_valid_mtbf (node->mtbf)) != OK_PARSER)
                            err_input (ERR_MTBF, *input_line_number);
                    }

                    if ((strncmp (str, "MTTR", 4) == 0) || (strncmp (str, "mttr", 4) == 0))
                        /* READ MTTR */
                    {
                        read_fields++;
                        sscanf_value = sscanf (input_string, "%s %f", str, &(node->mttr));
                        if ((sscanf_value < 2) || (sscanf_value == EOF))
                            err_input (ERR_SCAN, *input_line_number);
                        if ((is_valid_mttr (node->mttr)) != OK_PARSER)
                            err_input (ERR_MTTR, *input_line_number);
                    }
                }
            }
        }
    }
    if (read_fields <= 0)
        err_input (NO_FIELDS, *input_line_number);
    return (OK_PARSER);
}

/****************************************************************************/
/* 03/27/01                                                                 */
/* FUNCTION READ_LINK_BLOCK: READ THE LINK BLOCK IN THE FILE                */
/****************************************************************************/
int read_link_block (struct link_block *p_link, FILE * input_stream, int *input_line_number) {
    int sscanf_value;
    int line_read = 0;
    int read_fields = 0;
    char str[STRING_LENGTH], input_string[STRING_LENGTH];
    strcpy (str, "QWERTY");
    free_link (p_link);
    while (str[0] != '}')
    {
        fgets (input_string, STRING_LENGTH + 1, input_stream);
        (*input_line_number)++;
        sscanf_value = sscanf (input_string, "%s", str);
        if ((sscanf_value == 0) || (sscanf_value == EOF))
            strcpy (str, input_string);
        if (line_read == 0)
            if (str[0] == '}')
                return (END_OF_LINK_BLOCK);
        if (is_comment (str) != YES_PARSER)
        {


            /**********************************************************************/
            /**********************************************************************/
            /**********************************************************************/
            /* IN CASE NEW ELEMENTS ARE ADDED THEY MUST BE READ IN THIS IF        */
            /**********************************************************************/
            /**********************************************************************/
            /**********************************************************************/

            line_read++;
            if ((strcmp (str, "UNIDIR") == 0) || (strcmp (str, "unidir") == 0))
            {
                sscanf_value = sscanf (input_string, "%s %s %s %s", p_link->type,
                                       p_link->name, p_link->from, p_link->to);
                if ((sscanf_value < 4) || (sscanf_value == EOF))
                    err_input (ERR_SCAN, *input_line_number);
                while (str[0] != '}')
                {
                    fgets (input_string, STRING_LENGTH + 1, input_stream);
                    (*input_line_number)++;
                    sscanf_value = sscanf (input_string, "%s", str);
                    if ((sscanf_value == 0) || (sscanf_value == EOF))
                        strcpy (str, input_string);
                    if (line_read == 0)
                        if (str[0] == '}')
                            return (END_OF_LINK_BLOCK);
                    if (is_comment (str) != YES_PARSER)
                    {


                        /**********************************************************************/
                        /**********************************************************************/
                        /**********************************************************************/
                        /* IN CASE NEW ELEMENTS ARE ADDED THEY MUST BE READ IN THIS IF        */
                        /**********************************************************************/
                        /**********************************************************************/
                        /**********************************************************************/

                        line_read++;

                        if ((strcmp (str, "ID") == 0) || (strcmp (str, "id") == 0))
                        {
                            read_fields++;
                            sscanf_value = sscanf (input_string, "%s %d", str, &(p_link->ID));
                            if ((sscanf_value < 2) || (sscanf_value == EOF))
                                err_input (ERR_SCAN, *input_line_number);
                            if (is_valid_linkID (p_link->ID) != OK_PARSER)
                                err_input (ERR_ID, *input_line_number);
                        }

                        if ((strcmp (str, "DISTANCE") == 0) || (strcmp (str, "distance") == 0))
                        {
                            read_fields++;
                            sscanf_value = sscanf (input_string, "%s %f", str, &(p_link->distance));
                            if ((sscanf_value < 2) || (sscanf_value == EOF))
                                err_input (ERR_SCAN, *input_line_number);
                            if (is_valid_link_distance (p_link->distance) != OK_PARSER)
                                err_input (ERR_DISTANCE, *input_line_number);
                            //			  p_link->distance = p_link->distance/10;
                            //			  cout << "ATTENTION: link distance devided by 10! " << p_link->distance << endl;

                        }

                        /*   ------------ Added by Marco Ghizzi 11/26/2002 -----------*/
                        if ((strcmp (str, "MAXBYTE") == 0) || (strcmp (str, "maxbyte") == 0))
                        {
                            read_fields++;
                            sscanf_value = sscanf (input_string, "%s %f", str, &(p_link->maxbyte));
                            if ((sscanf_value < 2) || (sscanf_value == EOF))
                                err_input (ERR_SCAN, *input_line_number);
                            if (is_valid_link_maxbyte (p_link->maxbyte) != OK_PARSER)
                                err_input (ERR_BER, *input_line_number);
                        }

                        if ((strcmp (str, "NUM_CHANNEL") == 0) || (strcmp (str, "num_channel") == 0))
                        {
                            read_fields++;
                            sscanf_value = sscanf (input_string, "%s %f", str, &(p_link->num_channel));
                            if ((sscanf_value < 2) || (sscanf_value == EOF))
                                err_input (ERR_SCAN, *input_line_number);
                            if (is_valid_link_maxbyte (p_link->num_channel) != OK_PARSER)
                                err_input (ERR_BER, *input_line_number);
                        }

                        if ((strcmp (str, "BITRATE") == 0) || (strcmp (str, "bitrate") == 0))
                        {
                            read_fields++;
                            sscanf_value = sscanf (input_string, "%s %f", str, &(p_link->bitrate));
                            if ((sscanf_value < 2) || (sscanf_value == EOF))
                                err_input (ERR_SCAN, *input_line_number);
                            if (is_valid_link_bitrate (p_link->bitrate) != OK_PARSER)
                                err_input (ERR_BER, *input_line_number);
                        }

                        /*   ----------------------  end  ----------------------------*/

                        if ((strcmp (str, "BER") == 0) || (strcmp (str, "ber") == 0))
                        {
                            read_fields++;
                            sscanf_value = sscanf (input_string, "%s %f", str, &(p_link->ber));
                            if ((sscanf_value < 2) || (sscanf_value == EOF))
                                err_input (ERR_SCAN, *input_line_number);
                            if (is_valid_link_ber (p_link->ber) != OK_PARSER)
                                err_input (ERR_BER, *input_line_number);
                        }

                        if ((strcmp (str, "DELAY") == 0) || (strcmp (str, "delay") == 0))
                        {
                            read_fields++;
                            sscanf_value = sscanf (input_string, "%s %f", str, &(p_link->delay));
                            if ((sscanf_value < 2) || (sscanf_value == EOF))
                                err_input (ERR_SCAN, *input_line_number);
                            if (is_valid_link_delay (p_link->delay) != OK_PARSER)
                                err_input (ERR_DELAY, *input_line_number);
                        }

                        if ((strcmp (str, "JITTER") == 0) || (strcmp (str, "jitter") == 0))
                        {
                            read_fields++;
                            sscanf_value = sscanf (input_string, "%s %f", str, &(p_link->jitter));
                            if ((sscanf_value < 2) || (sscanf_value == EOF))
                                err_input (ERR_SCAN, *input_line_number);
                            if (is_valid_link_jitter (p_link->jitter) != OK_PARSER)
                                err_input (ERR_JITTER, *input_line_number);
                        }

                        if ((strcmp (str, "BW") == 0) || (strcmp (str, "bw") == 0))
                        {
                            read_fields++;
                            sscanf_value = sscanf (input_string, "%s %f", str, &(p_link->bw));
                            if ((sscanf_value < 2) || (sscanf_value == EOF))
                                err_input (ERR_SCAN, *input_line_number);
                            if (is_valid_link_bandwidth (p_link->bw) != OK_PARSER)
                                err_input (ERR_BW, *input_line_number);
                        }

                        if ((strcmp (str, "MFP") == 0) || (strcmp (str, "mfp") == 0))
                        {
                            read_fields++;
                            sscanf_value = sscanf (input_string, "%s %f", str, &(p_link->mfp));
                            if ((sscanf_value < 2) || (sscanf_value == EOF))
                                err_input (ERR_SCAN, *input_line_number);
                            if (is_valid_link_mfp (p_link->mfp) != OK_PARSER)
                                err_input (ERR_MFP, *input_line_number);
                        }

                        if ((strcmp (str, "MTBF") == 0) || (strcmp (str, "mtbf") == 0))
                        {
                            read_fields++;
                            sscanf_value = sscanf (input_string, "%s %f", str, &(p_link->mtbf));
                            if ((sscanf_value < 2) || (sscanf_value == EOF))
                                err_input (ERR_SCAN, *input_line_number);
                            if (is_valid_link_mtbf (p_link->mtbf) != OK_PARSER)
                                err_input (ERR_MTBF, *input_line_number);
                        }

                        if ((strcmp (str, "MTTR") == 0) || (strcmp (str, "mttr") == 0))
                        {
                            read_fields++;
                            sscanf_value = sscanf (input_string, "%s %f", str, &(p_link->mttr));
                            if ((sscanf_value < 2) || (sscanf_value == EOF))
                                err_input (ERR_SCAN, *input_line_number);
                            if (is_valid_link_mttr (p_link->mttr) != OK_PARSER)
                                err_input (ERR_MTTR, *input_line_number);
                        }

                        if ((strcmp (str, "FIBER_RATIO") == 0) || (strcmp (str, "fiber_ratio") == 0))
                        {
                            read_fields++;
                            sscanf_value = sscanf (input_string, "%s %d", str, &(p_link->fiber_ratio));
                            if ((sscanf_value < 2) || (sscanf_value == EOF))
                                err_input (ERR_SCAN, *input_line_number);
                            if (is_valid_link_fiber_ratio (p_link->fiber_ratio) != OK_PARSER)
                                err_input (ERR_FIBER_RATIO, *input_line_number);
                        }


                        if ((strcmp (str, "FIBER_LIST") == 0) || (strcmp (str, "fiber_list") == 0))
                        {
                            read_fields++;
                            if (is_valid_link_fiber_ratio (p_link->fiber_ratio) != OK_PARSER)
                                err_input (ERR_FIBER_RATIO, *input_line_number);
                            if (allocate_fiber_list_array (p_link) != OK_PARSER)
                                exit (ERR_MEM_PARSER);
                            int ii;
                            for (ii = 0;; ii++)
                            {
                                if (read_fiber_list_block (p_link->fiber_array, ii,
                                                           input_line_number, input_stream) == END_OF_FIBER_LIST)
                                    break;

                            }
                            if (ii != get_link_fiber_ratio (*p_link))
                            {
                                fprintf (stderr, "NUMBER OF FIBERS: %d\nFIBER_RATIO %d\n",
                                         ii, get_link_fiber_ratio (*p_link));
                                err_input (ERR_FIBER_LIST, (*input_line_number));
                            }
                        }
                    }		/*closes if (is_comment */
                }		/*closes if(strcmp(.... */
            }			/*closes while */
        }
    }
    if (read_fields <= 0)
        err_input (NO_FIELDS, *input_line_number);
    return (OK_PARSER);
}


/***************************************************************************/
/* 04/25/01                                                                */
/* FUNCTION GET_NODE_XPOS                                                  */
/* RETURNS THE NODE POSITION OR MAXFLOAT ON ERROR                          */
/***************************************************************************/
float
get_node_xpos (struct node_block node)
{
    if (is_valid_xpos (node.xpos) == OK_PARSER)
        return (node.xpos);
    return (MAXFLOAT);
}

/***************************************************************************/
/* 04/25/01                                                                */
/* FUNCTION GET_NODE_YPOS                                                  */
/* RETURNS THE NODE POSITION OR MAXFLOAT ON ERROR                          */
/***************************************************************************/
float
get_node_ypos (struct node_block node)
{
    if (is_valid_xpos (node.ypos) == OK_PARSER)
        return (node.ypos);
    return (MAXFLOAT);
}

/***************************************************************************/
/* 04/25/01                                                                */
/* FUNCTION GET_NODE_ID RETURNS NODE ID OR -1 ON INVALID NODE ID           */
/***************************************************************************/
int
get_node_ID (struct node_block node)
{
    if (is_valid_nodeID (node.ID) == OK_PARSER)
        return (node.ID);
    return (-1);
}


/**************************************************************************/
/* 04/25/01                                                               */
/* FUNCTION GET_NODE_NAME RETURNS NAME OR NULL POINTER ON INVALID NAME    */
/* THE MEMORY NECESSARY TO HOLD THE NAME IS ALLOCATED HERE                */
/**************************************************************************/
char *
get_node_name (struct node_block node)
{
    char *name;
    int n;
    n = strlen (node.name);
    if (n > 0)
        if (node.name[n - 1] == ';')
            node.name[n - 1] = '\0';
    name = (char *) malloc (n * sizeof (char));
    strcpy (name, node.name);
    return (name);
}

/***************************************************************************/
/* 04/25/01                                                                */
/* FUNCTION GET_NODE_TYPE RETURNS TYPE OR NULL ON INVALID TYPE             */
/* THE MEMORY NECESSARY TO HOLD THE NAME IS ALLOCATED HERE                 */
/***************************************************************************/
char *
get_node_type (struct node_block node)
{
    char *type;
    int n;
    n = strlen (node.type);
    if (n > 0)
        if (node.type[n - 1] == ';')
            node.type[n - 1] = '\0';
    type = (char *) malloc (n * sizeof (char));
    strcpy (type, node.type);
    return (type);
}

/****************************************************************************/
/* 04/25/01                                                                 */
/* FUNCTION GET_NODE_TRX_BITRATE RETURNS TRANSMISSION BIT RATE OF THE NODE  */
/* OR -1.0 ON INVALID TRANSMISSION BIT_RATE                                 */
/****************************************************************************/
float
get_node_trx_bitrate (struct node_block node)
{
    if (is_valid_trx_bitrate (node.trx_bitrate) == OK_PARSER)
        return (node.trx_bitrate);
    return (-1.0);
}

/****************************************************************************/
/* 04/25/01                                                                 */
/* FUNCTION GET_NODE_REC_SENSITIVITY RETURNS RECEIVER SENSITIVITY           */
/* OR -1.0 ON INVALID VALUE                                                 */
/****************************************************************************/
float
get_node_rec_sensitivity (struct node_block node)
{
    if (is_valid_rec_sensitivity (node.rec_sensitivity) == OK_PARSER)
        return (node.rec_sensitivity);
    return (-1.0);
}

/****************************************************************************/
/* 04/25/01                                                                 */
/* FUNCTION GET_NODE_BER RETURNS NODE BER OR -1.0 ON INVALID VALUE          */
/****************************************************************************/
float
get_node_ber (struct node_block node)
{
    if (is_valid_ber (node.ber) == OK_PARSER)
        return (node.ber);
    return (-1.0);
}


/****************************************************************************/
/* 04/25/01                                                                 */
/* FUNCTION GET_NODE_DELAY RETURNS NODE DELAY OR -1.0 ON INVALID VALUE      */
/****************************************************************************/
float
get_node_delay (struct node_block node)
{
    if (is_valid_delay (node.delay) == OK_PARSER)
        return (node.delay);
    return (-1.0);
}

/****************************************************************************/
/* 04/25/01                                                                 */
/* FUNCTION GET_NODE_JITTER RETURNS NODE JITTER OR -1.0 ON INVALID VALUE    */
/****************************************************************************/
float
get_node_jitter (struct node_block node)
{
    if (is_valid_jitter (node.jitter) == OK_PARSER)
        return (node.jitter);
    return (-1.0);
}

/****************************************************************************/
/* 04/25/01                                                                 */
/* FUNCTION GET_NODE_BW RETURNS NODE BW OR -1.0 ON INVALID VALUE            */
/****************************************************************************/
float
get_node_bw (struct node_block node)
{
    if (is_valid_bw (node.bw) == OK_PARSER)
        return (node.bw);
    return (-1.0);
}

/****************************************************************************/
/* 04/25/01                                                                 */
/* FUNCTION GET_NODE_MTBF RETURNS NODE MTBF OR -1.0 ON INVALID VALUE        */
/****************************************************************************/
float
get_node_mtbf (struct node_block node)
{
    if (is_valid_mtbf (node.mtbf) == OK_PARSER)
        return (node.mtbf);
    return (-1.0);
}

/****************************************************************************/
/* 04/25/01                                                                 */
/* FUNCTION GET_NODE_MTTR RETURNS NODE MTTR OR -1.0 ON INVALID VALUE     */
/****************************************************************************/
float
get_node_mttr (struct node_block node)
{
    if (is_valid_mttr (node.mttr) == OK_PARSER)
        return (node.mttr);
    return (-1.0);
}


/****************************************************************************/
/* 04/25/01                                                                 */
/* FUNCTION GET_NODE_OP_TYPE RETURNS NODE OP_TYPE OR NULL ON INVALID VALUE  */
/****************************************************************************/
char *
get_node_op_type (struct node_block node)
{
    char *type;
    int n;
    if (is_valid_op_type (node.op_type) != OK_PARSER)
        return (NULL);
    n = strlen (node.op_type);
    if (n > 0)
        if (node.op_type[n - 1] == ';')
            node.op_type[n - 1] = '\0';
    type = (char *) malloc (n * sizeof (char));
    strcpy (type, node.op_type);
    return (type);
}

/****************************************************************************/
/* 04/25/01                                                                 */
/* FUNCTION GET_NODE_STATUS RETURNS NODE STATUS OR NULL ON INVALID VALUE    */
/****************************************************************************/
char *
get_node_status (struct node_block node)
{
    char *type;
    int n;
    if (is_valid_status (node.status) != OK_PARSER)
        return (NULL);
    n = strlen (node.status);
    if (n > 0)
        if (node.status[n - 1] == ';')
            node.status[n - 1] = '\0';
    type = (char *) malloc (n * sizeof (char));
    strcpy (type, node.status);
    return (type);
}


/****************************************************************************/
/* 04/26/01                                                                 */
/* FUNCTION GET_LINK_TYPE RETURNS LINK TYPE OR NULL ON INVALID VALUE        */
/****************************************************************************/
char *
get_link_type (struct link_block link)
{
    char *type;
    int n;
    n = strlen (link.type);
    if (n > 0)
        if (link.type[n - 1] == ';')
            link.type[n - 1] = '\0';
    type = (char *) malloc (n * sizeof (char));
    strcpy (type, link.type);
    return (type);
}

/****************************************************************************/
/* 04/26/01                                                                 */
/* FUNCTION GET_LINK_NAME RETURNS LINK NAME OR NULL ON INVALID VALUE        */
/****************************************************************************/
char *
get_link_name (struct link_block link)
{
    char *type;
    int n;
    n = strlen (link.name);
    if (n > 0)
        if (link.name[n - 1] == ';')
            link.name[n - 1] = '\0';
    type = (char *) malloc (n * sizeof (char));
    strcpy (type, link.name);
    return (type);
}

/****************************************************************************/
/* 04/26/01                                                                 */
/* FUNCTION GET_LINK_FROM RETURNS THE NODE NAME FROM WHICH THE LINK         */
/* ORIGINATES OR NULL ON INVALID VALUE                                      */
/****************************************************************************/
char *
get_link_from (struct link_block link)
{
    char *type;
    int n;
    n = strlen (link.from);
    if (n > 0)
        if (link.from[n - 1] == ';')
            link.from[n - 1] = '\0';
    type = (char *) malloc (n * sizeof (char));
    strcpy (type, link.from);
    return (type);
}

/****************************************************************************/
/* 04/26/01                                                                 */
/* FUNCTION GET_LINK_TO RETURNS THE NODE NAME TO WHICH THE LINK             */
/* DESTINATED OR NULL ON INVALID VALUE                                      */
/****************************************************************************/
char *
get_link_to (struct link_block link)
{
    char *type;
    int n;
    n = strlen (link.to);
    if (n > 0)
        if (link.to[n - 1] == ';')
            link.to[n - 1] = '\0';
    type = (char *) malloc (n * sizeof (char));
    strcpy (type, link.to);
    return (type);
}



/***************************************************************************/
/* 04/26/01                                                                */
/* FUNCTION GET_LINK_ID RETURNS LINK ID OR -1 ON INVALID LINK ID           */
/***************************************************************************/
int
get_link_ID (struct link_block link)
{
    if (is_valid_linkID (link.ID) == OK_PARSER)
        return (link.ID);
    return (-1);
}

/***************************************************************************/
/* 04/26/01                                                                */
/* FUNCTION GET_LINK_DISTANCE RETURNS LINK DISTANCE OR -1.0 ON             */
/* INVALID LINK DISTANCE                                                   */
/***************************************************************************/
float
get_link_distance (struct link_block link)
{
    if (is_valid_link_distance (link.distance) == OK_PARSER)
        return (link.distance);
    return (-1.0);
}

/***************************************************************************/
/* 04/26/01                                                                */
/* FUNCTION GET_LINK_BER RETURNS LINK BER OR -1.0 ON                       */
/* INVALID LINK BER                                                        */
/***************************************************************************/
float
get_link_ber (struct link_block link)
{
    if (is_valid_link_ber (link.ber) == OK_PARSER)
        return (link.ber);
    return (-1.0);
}

/***************************************************************************/
/* 04/26/01                                                                */
/* FUNCTION GET_LINK_DELAY RETURNS LINK DELAY OR -1.0 ON                   */
/* INVALID LINK DELAY                                                      */
/***************************************************************************/
float
get_link_delay (struct link_block link)
{
    if (is_valid_link_delay (link.delay) == OK_PARSER)
        return (link.delay);
    return (-1.0);
}

/***************************************************************************/
/* 04/26/01                                                                */
/* FUNCTION GET_LINK_JITTER RETURNS LINK JITTER OR -1.0 ON                 */
/* INVALID LINK DISTANCE                                                   */
/***************************************************************************/
float
get_link_jitter (struct link_block link)
{
    if (is_valid_link_jitter (link.jitter) == OK_PARSER)
        return (link.jitter);
    return (-1.0);
}

/***************************************************************************/
/* 04/26/01                                                                */
/* FUNCTION GET_LINK_BW RETURNS LINK BANDWIDTH OR -1.0 ON                  */
/* INVALID LINK BW                                                         */
/***************************************************************************/
float
get_link_bw (struct link_block link)
{
    if (is_valid_link_bandwidth (link.bw) == OK_PARSER)
        return (link.bw);
    return (-1.0);
}

/***************************************************************************/
/* 04/26/01                                                                */
/* FUNCTION GET_LINK_MFP RETURNS LINK MFP OR -1.0 ON                       */
/* INVALID LINK MFP                                                        */
/***************************************************************************/
float
get_link_mfp (struct link_block link)
{
    if (is_valid_link_mfp (link.mfp) == OK_PARSER)
        return (link.mfp);
    return (-1.0);
}

/***************************************************************************/
/* 04/26/01                                                                */
/* FUNCTION GET_LINK_MTBF RETURNS LINK MTBF OR -1.0 ON                     */
/* INVALID LINK MTBF                                                       */
/***************************************************************************/
float
get_link_mtbf (struct link_block link)
{
    if (is_valid_link_mtbf (link.mtbf) == OK_PARSER)
        return (link.mtbf);
    return (-1.0);
}

/***************************************************************************/
/* 04/26/01                                                                */
/* FUNCTION GET_LINK_MTTR RETURNS LINK MTTR OR -1.0 ON                     */
/* INVALID LINK MTTR                                                       */
/***************************************************************************/
float
get_link_mttr (struct link_block link)
{
    if (is_valid_link_mttr (link.mttr) == OK_PARSER)
        return (link.mttr);
    return (-1.0);
}

/***************************************************************************/
/* 04/26/01                                                                */
/* FUNCTION GET_LINK_FIBER_RATIO RETURNS LINK FIBER_RATIO OR -1 ON       */
/* INVALID LINK FIBER_RATIO                                                */
/***************************************************************************/
int
get_link_fiber_ratio (struct link_block link)
{
    if (is_valid_link_fiber_ratio (link.fiber_ratio) == OK_PARSER)
        return (link.fiber_ratio);
    return (-1);
}

/***************************************************************************/
/* 04/26/01                                                                */
/* FUNCTION GET_LINK_FIBER_NAME RETURNS LINK FIBER NAME OF THE I-TH FIBER  */
/* OR NULL ON INVALID NAME                                                 */
/***************************************************************************/
char *
get_link_fiber_name (struct link_block link, int i)
{
    if (i >= get_link_fiber_ratio (link))
    {
        fprintf (stderr, "INVALID VALUE %d GREATER THAN FIBER RATIO %d\n",
                 i, get_link_fiber_ratio (link));
        return (NULL);
    }
    char *type;
    int n;
    n = strlen (link.fiber_array[i].name);
    if (n > 0)
        if (link.fiber_array[i].name[n - 1] == ';')
            link.fiber_array[i].name[n - 1] = '\0';
    type = (char *) malloc (n * sizeof (char));
    strcpy (type, link.fiber_array[i].name);
    return (type);
}

/***************************************************************************/
/* 04/26/01                                                                */
/* FUNCTION GET_LINK_FIBER_TYPE RETURNS LINK FIBER TYPE OF THE I-TH FIBER  */
/* OR NULL ON INVALID TYPE                                                 */
/***************************************************************************/
char *
get_link_fiber_type (struct link_block link, int i)
{
    if (i >= get_link_fiber_ratio (link))
    {
        fprintf (stderr, "INVALID VALUE %d GREATER THAN FIBER RATIO %d\n",
                 i, get_link_fiber_ratio (link));
        return (NULL);
    }
    char *type;
    int n;
    n = strlen (link.fiber_array[i].type);
    if (n > 0)
        if (link.fiber_array[i].type[n - 1] == ';')
            link.fiber_array[i].type[n - 1] = '\0';
    type = (char *) malloc (n * sizeof (char));
    strcpy (type, link.fiber_array[i].type);
    return (type);
}


/***************************************************************************/
/* 04/26/01                                                                */
/* FUNCTION GET_LINK_FIBER_STATUS RETURNS FIBER STATUS OF THE I-TH FIBER   */
/* OR NULL ON INVALID NAME                                                 */
/***************************************************************************/
char *
get_link_fiber_status (struct link_block link, int i)
{
    if (i >= get_link_fiber_ratio (link))
    {
        fprintf (stderr, "INVALID VALUE %d GREATER THAN FIBER RATIO %d\n",
                 i, get_link_fiber_ratio (link));
        return (NULL);
    }
    char *type;
    int n;
    if (is_valid_fiber_list_status (link.fiber_array[i].status) != OK_PARSER)
        return (NULL);
    n = strlen (link.fiber_array[i].status);
    if (n > 0)
        if (link.fiber_array[i].status[n - 1] == ';')
            link.fiber_array[i].status[n - 1] = '\0';
    type = (char *) malloc (n * sizeof (char));
    strcpy (type, link.fiber_array[i].status);
    return (type);
}


/***************************************************************************/
/* 04/26/01                                                                */
/* FUNCTION GET_LINK_FIBER_OP_TYPE RETURNS FIBER OP_TYPE OF THE I-TH FIBER */
/* OR NULL ON INVALID OP_TYPE                                              */
/***************************************************************************/
char *
get_link_fiber_op_type (struct link_block link, int i)
{
    if (i >= get_link_fiber_ratio (link))
    {
        fprintf (stderr, "INVALID VALUE %d GREATER THAN FIBER RATIO %d\n",
                 i, get_link_fiber_ratio (link));
        return (NULL);
    }
    char *type;
    int n;
    if (is_valid_fiber_list_op_type (link.fiber_array[i].op_type) != OK_PARSER)
        return (NULL);
    n = strlen (link.fiber_array[i].op_type);
    if (n > 0)
        if (link.fiber_array[i].op_type[n - 1] == ';')
            link.fiber_array[i].op_type[n - 1] = '\0';
    type = (char *) malloc (n * sizeof (char));
    strcpy (type, link.fiber_array[i].op_type);
    return (type);
}

/***************************************************************************/
/* 04/26/01                                                                */
/* FUNCTION GET_LINK_FIBER_ID RETURNS FIBER ID OF THE I-TH FIBER           */
/* OR -1 ON INVALID ID                                                     */
/***************************************************************************/
int
get_link_fiber_ID (struct link_block link, int i)
{
    if (i >= get_link_fiber_ratio (link))
    {
        fprintf (stderr, "INVALID VALUE %d GREATER THAN FIBER RATIO %d\n",
                 i, get_link_fiber_ratio (link));
        return (-1);
    }
    if (is_valid_fiber_list_ID (link.fiber_array[i].ID) == OK_PARSER)
        return (link.fiber_array[i].ID);
    return (-1);
}

/***************************************************************************/
/* 04/26/01                                                                */
/* FUNCTION GET_LINK_FIBER_LAMBDA_RATIO RETURNS FIBER LAMBDA_RATIO         */
/*  OF THE I-TH FIBER OR -1 ON INVALID LAMBDA_RATIO                        */
/***************************************************************************/
int
get_link_fiber_lambda_ratio (struct link_block link, int i)
{
    if (i >= get_link_fiber_ratio (link))
    {
        fprintf (stderr, "INVALID VALUE %d GREATER THAN FIBER RATIO %d\n",
                 i, get_link_fiber_ratio (link));
        return (-1);
    }
    if (is_valid_fiber_list_lambda_ratio (link.fiber_array[i].lambda_ratio) == OK_PARSER)
        return (link.fiber_array[i].lambda_ratio);
    return (-1);
}

/***************************************************************************/
/* 04/26/01                                                                */
/* FUNCTION GET_LINK_FIBER_AMPLY_RATIO RETURNS FIBER AMPLY_RATIO           */
/*  OF THE I-TH FIBER OR -1 ON INVALID AMPLY_RATIO                         */
/***************************************************************************/
int
get_link_fiber_amply_ratio (struct link_block link, int i)
{
    if (i >= get_link_fiber_ratio (link))
    {
        fprintf (stderr, "INVALID VALUE %d GREATER THAN FIBER RATIO %d\n",
                 i, get_link_fiber_ratio (link));
        return (-1);
    }
    if (is_valid_fiber_amplifier_ratio (link.fiber_array[i].amplifier_ratio)
            == OK_PARSER)
        return (link.fiber_array[i].amplifier_ratio);
    return (-1);
}

/*****************************************************************************/
/* 04/26/01                                                                  */
/* FUNCTION GET_LINK_FIBER_LAMBDA_NAME RETURNS LAMBDA NAME OF THE J-TH       */
/* LAMBDA ON THE I-TH FIBER OR NULL ON INVALID VALUE                         */
/*****************************************************************************/
char *
get_link_fiber_lambda_name (struct link_block link, int i, int j)
{
    if (i >= get_link_fiber_ratio (link))
    {
        fprintf (stderr, "INVALID VALUE %d GREATER THAN FIBER RATIO %d\n",
                 i, get_link_fiber_ratio (link));
        return (NULL);
    }
    if (j >= get_link_fiber_lambda_ratio (link, i))
    {
        fprintf (stderr, "INVALID VALUE %d GREATER THAN LAMBDA_RATIO %d\n",
                 j, get_link_fiber_lambda_ratio (link, i));
        return (NULL);
    }
    char *type;
    int n;
    n = strlen (link.fiber_array[i].lambda_array[j].lambda);
    if (n > 0)
        if (link.fiber_array[i].lambda_array[j].lambda[n - 1] == ';')
            link.fiber_array[i].lambda_array[j].lambda[n - 1] = '\0';
    type = (char *) malloc (n * sizeof (char));
    strcpy (type, link.fiber_array[i].lambda_array[j].lambda);
    return (type);
}

/*****************************************************************************/
/* 04/26/01                                                                  */
/* FUNCTION GET_LINK_FIBER_LAMBDA_OP_TYPE RETURNS LAMBDA OP_TYPE OF THE J-TH */
/* LAMBDA ON THE I-TH FIBER OR NULL ON INVALID OP_TYPE                       */
/*****************************************************************************/
char *
get_link_fiber_lambda_op_type (struct link_block link, int i, int j)
{
    if (i >= get_link_fiber_ratio (link))
    {
        fprintf (stderr, "INVALID VALUE %d GREATER THAN FIBER RATIO %d\n",
                 i, get_link_fiber_ratio (link));
        return (NULL);
    }
    if (j >= get_link_fiber_lambda_ratio (link, i))
    {
        fprintf (stderr, "INVALID VALUE %d GREATER THAN LAMBDA_RATIO %d\n",
                 j, get_link_fiber_lambda_ratio (link, i));
        return (NULL);
    }
    if (is_valid_lambda_op_type (link.fiber_array[i].lambda_array[j].op_type) !=
            OK_PARSER)
        return (NULL);
    char *type;
    int n;
    n = strlen (link.fiber_array[i].lambda_array[j].op_type);
    if (n > 0)
        if (link.fiber_array[i].lambda_array[j].op_type[n - 1] == ';')
            link.fiber_array[i].lambda_array[j].op_type[n - 1] = '\0';
    type = (char *) malloc (n * sizeof (char));
    strcpy (type, link.fiber_array[i].lambda_array[j].op_type);
    return (type);
}

/*****************************************************************************/
/* 04/26/01                                                                  */
/* FUNCTION GET_LINK_FIBER_LAMBDA_STATUS RETURNS LAMBDA STATUS OF THE J-TH   */
/* LAMBDA ON THE I-TH FIBER OR NULL ON INVALID STATUS                        */
/*****************************************************************************/
char *
get_link_fiber_lambda_status (struct link_block link, int i, int j)
{
    if (i >= get_link_fiber_ratio (link))
    {
        fprintf (stderr, "INVALID VALUE %d GREATER THAN FIBER RATIO %d\n",
                 i, get_link_fiber_ratio (link));
        return (NULL);
    }
    if (j >= get_link_fiber_lambda_ratio (link, i))
    {
        fprintf (stderr, "INVALID VALUE %d GREATER THAN LAMBDA_RATIO %d\n",
                 j, get_link_fiber_lambda_ratio (link, i));
        return (NULL);
    }
    if (is_valid_lambda_status (link.fiber_array[i].lambda_array[j].status) !=
            OK_PARSER)
        return (NULL);
    char *type;
    int n;
    n = strlen (link.fiber_array[i].lambda_array[j].status);
    if (n > 0)
        if (link.fiber_array[i].lambda_array[j].status[n - 1] == ';')
            link.fiber_array[i].lambda_array[j].status[n - 1] = '\0';
    type = (char *) malloc (n * sizeof (char));
    strcpy (type, link.fiber_array[i].lambda_array[j].status);
    return (type);
}

/*****************************************************************************/
/* 04/26/01                                                                  */
/* FUNCTION GET_LINK_FIBER_LAMBDA_ID RETURNS LAMBDA ID OF THE J-TH           */
/* LAMBDA ON THE I-TH FIBER OR -1 ON INVALID ID                              */
/*****************************************************************************/
int
get_link_fiber_lambda_ID (struct link_block link, int i, int j)
{
    if (i >= get_link_fiber_ratio (link))
    {
        fprintf (stderr, "INVALID VALUE %d GREATER THAN FIBER RATIO %d\n",
                 i, get_link_fiber_ratio (link));
        return (-1);
    }
    if (j >= get_link_fiber_lambda_ratio (link, i))
        return (-1);
    if (is_valid_lambda_ID (link.fiber_array[i].lambda_array[j].lambdaID)
            != OK_PARSER)
        return (-1);
    return (link.fiber_array[i].lambda_array[j].lambdaID);
}

/*****************************************************************************/
/* 04/26/01                                                                  */
/* FUNCTION GET_LINK_AMPLY_POS RETURNS POSITION OF THE J-TH                  */
/* AMPLIFIER ON THE I-TH FIBER OR -1.0 ON INVALID ID                         */
/*****************************************************************************/
float
get_link_fiber_amply_pos (struct link_block link, int i, int j)
{
    if (i >= get_link_fiber_ratio (link))
    {
        fprintf (stderr, "INVALID VALUE %d GREATER THAN FIBER RATIO %d\n",
                 i, get_link_fiber_ratio (link));
        return (-1.0);
    }
    if (j >= get_link_fiber_amply_ratio (link, i))
    {
        fprintf (stderr, "INVALID VALUE %d GREATER THAN AMPLIFIER_RATIO %d\n",
                 j, get_link_fiber_amply_ratio (link, i));
        return (-1.0);
    }
    if (is_valid_amply_pos (link.fiber_array[i].amply_array[j].position,
                            get_link_distance (link)) !=
            OK_PARSER)
        return (-1.0);
    return (link.fiber_array[i].amply_array[j].position);
}


/*****************************************************************************/
/* 04/26/01                                                                  */
/* FUNCTION GET_LINK_AMPLY_TYPE RETURNS TYPE OF THE J-TH                     */
/* AMPLIFIER ON THE I-TH FIBER OR NULL ON INVALID TYPE                       */
/*****************************************************************************/
char *
get_link_fiber_amply_type (struct link_block link, int i, int j)
{
    if (i >= get_link_fiber_ratio (link))
    {
        fprintf (stderr, "INVALID VALUE %d GREATER THAN FIBER RATIO %d\n",
                 i, get_link_fiber_ratio (link));
        return (NULL);
    }
    if (j >= get_link_fiber_amply_ratio (link, i))
    {
        fprintf (stderr, "INVALID VALUE %d GREATER THAN AMPLIFIER_RATIO %d\n",
                 j, get_link_fiber_amply_ratio (link, i));
        return (NULL);
    }
    char *type;
    int n;
    n = strlen (link.fiber_array[i].amply_array[j].amplifier_type);
    if (n > 0)
        if (link.fiber_array[i].amply_array[j].amplifier_type[n - 1] == ';')
            link.fiber_array[i].amply_array[j].amplifier_type[n - 1] = '\0';
    type = (char *) malloc (n * sizeof (char));
    strcpy (type, link.fiber_array[i].amply_array[j].amplifier_type);
    return (type);
}

/*****************************************************************************/
/* 04/26/01                                                                  */
/* FUNCTION GET_LINK_AMPLY_NAME RETURNS NAME OF THE J-TH                     */
/* AMPLIFIER ON THE I-TH FIBER OR NULL ON INVALID NAME                       */
/*****************************************************************************/
char *
get_link_fiber_amply_name (struct link_block link, int i, int j)
{
    if (i >= get_link_fiber_ratio (link))
    {
        fprintf (stderr, "INVALID VALUE %d GREATER THAN FIBER RATIO %d\n",
                 i, get_link_fiber_ratio (link));
        return (NULL);
    }
    if (j >= get_link_fiber_amply_ratio (link, i))
    {
        fprintf (stderr, "INVALID VALUE %d GREATER THAN AMPLIFIER_RATIO %d\n",
                 j, get_link_fiber_amply_ratio (link, i));
        return (NULL);
    }
    char *type;
    int n;
    n = strlen (link.fiber_array[i].amply_array[j].amplifier_name);
    if (n > 0)
        if (link.fiber_array[i].amply_array[j].amplifier_name[n - 1] == ';')
            link.fiber_array[i].amply_array[j].amplifier_name[n - 1] = '\0';
    type = (char *) malloc (n * sizeof (char));
    strcpy (type, link.fiber_array[i].amply_array[j].amplifier_name);
    return (type);
}


/***************************************************************************/
/* 05/04/01                                                                */
/* FUNCTION INIT_DEMAND: INITIALIZE DEMAND VALUES                          */
/***************************************************************************/
int
init_demand (struct demand_block *p_demand)
{
    strcpy (p_demand->name, "\0");
    strcpy (p_demand->type, "\0");
    strcpy (p_demand->source, "\0");
    strcpy (p_demand->destination, "\0");
    p_demand->demand = -1;
    strcpy (p_demand->demand_type, "\0");
    p_demand->demand_class = -1;
    return (OK_PARSER);
}




/****************************************************************************/
/* 05/04/01                                                                 */
/* FUNCTIONS IS_VALID_SOURCE RETURNS OK_PARSER IF SOURCE IS A VALID RANGE   */
/****************************************************************************/
int
is_valid_source (char *source)
{
    if (strcmp (source, "\0"))
        return OK_PARSER;
    else
    {
        fprintf (stderr, "source = %s\n", source);
        fprintf (stderr, "If source = %c -> check function is_valid_source\n", '\0');
        fprintf (stderr, "INVALID SOURCE: %s\n", source);
        return (NOT_OK_PARSER);
    }
}

/****************************************************************************/
/* 05/04/01                                                                 */
/* FUNCTIONS IS_VALID_DEMAND RETURNS OK_PARSER IF DEMAND IS A VALID RANGE   */
/****************************************************************************/
int
is_valid_destination (char *destination)
{
    if (strcmp (destination, "\0"))
        return OK_PARSER;
    else
    {
        fprintf (stderr, "destination = %s\n", destination);
        fprintf (stderr, "If destination = %c -> check function is_valid_destination\n", '\0');
        fprintf (stderr, "INVALID DESTINATION: %s\n", destination);
        return (NOT_OK_PARSER);
    }
}

/****************************************************************************/
/* 05/04/01                                                                 */
/* FUNCTIONS IS_VALID_DEMAND RETURNS OK_PARSER IF DEMAND IS A VALID RANGE  */
/****************************************************************************/
int
is_valid_demand (float demand)
{
    if (demand > 0)
        return OK_PARSER;
    else
    {
        fprintf (stderr, "DEMAND = %f\n", demand);
        fprintf (stderr, "If DEMAND = -1 -> check function is_valid_demand\n");
        fprintf (stderr, "INVALID DEMAND CLASS: %f\n", demand);
        return (NOT_OK_PARSER);
    }
}

/****************************************************************************/
/* 05/04/01                                                                 */
/* FUNCTIONS IS_VALID_DEMAND_TYPE RETURNS OK_PARSER IF TYPE IS A VALID RANGE  */
/****************************************************************************/
int
is_valid_demand_type (char *demand_type)
{
    if (strncmp (demand_type, "STS-1", 5) == 0)
        return OK_PARSER;
    else
    {
        fprintf (stderr, "Demand_type = %s\n", demand_type);
        fprintf (stderr, "If demand_type =  -> check function is_valid_type \n");
        fprintf (stderr, "INVALID DEMAND TYPE: %s\n", demand_type);
        return (NOT_OK_PARSER);
    }
}

/****************************************************************************/
/* 05/04/01                                                                 */
/* FUNCTIONS IS_VALID_CLASS RETURNS OK_PARSER IF CLASS IS A VALID RANGE  */
/****************************************************************************/
int
is_valid_class (int demand_class)
{
    if (demand_class >= 0)
        return OK_PARSER;
    else
    {
        fprintf (stderr, "Class = %d\n", demand_class);
        fprintf (stderr, "If class = -1 -> check function is_valid_class\n");
        fprintf (stderr, "INVALID DEMAND CLASS: %d\n", demand_class);
        return (NOT_OK_PARSER);
    }
}



/*************************************************************************/
/* 05/03/01                                                              */
/* FUNCTION ALLOCATE_LIGHTPATH_LIST_ARRAY ALLOCATES THE ARRAY FOR THE    */
/* LIGHTPATH                                                             */
/*************************************************************************/
/*************** NOT USED: LIGHPATHS LIST TO BE READ!!!! ***************/
/*int allocate_lightpath_list_array(struct lightpath_block *p_lightpath)
   {
   if (((p_lightpath->lightpath_array) = (struct lightpath_list *)
   malloc (p_lightpath->fiber_ratio*sizeof(struct lightpath_list)))==NULL)
   exit(ERR_MEM_PARSER);
   else return(OK_PARSER);
   }
 */
/****************************************************************************/
/* 05/03/01                                                                 */
/* FUNCTION READ_DEMAND_BLOCK: READS A BLOCK OF DATA AND PUTS IT IN THE     */
/* APPROPRIATE STRUCTURE                                                    */
/****************************************************************************/
int
read_demand_block (struct demand_block *demand,
                   FILE * input_stream, int *input_line_number)
{
    int sscanf_value;
    int line_read = 0;
    int read_fields = 0;
    char str[STRING_LENGTH], input_string[STRING_LENGTH];
    strcpy (str, "QWERTY");
    init_demand (demand);
    while (str[0] != '}')
    {
        fgets (input_string, STRING_LENGTH + 1, input_stream);
        (*input_line_number)++;
        sscanf_value = sscanf (input_string, "%s", str);
        if ((sscanf_value == 0) || (sscanf_value == EOF))
            strcpy (str, input_string);
        if (line_read == 0)
            if (str[0] == '}')
                return (END_OF_DEMAND_BLOCK);
        /**********************************************************************/
        /**********************************************************************/
        /**********************************************************************/
        /* IN CASE NEW ELEMENTS ARE ADDED THEY MUST BE READ IN THIS IF        */
        /**********************************************************************/
        /**********************************************************************/
        /**********************************************************************/
        if (is_comment (str) != YES_PARSER)
            /* BLOCK IS A DEMAND BLOCK */
        {
            line_read++;
            if ((strcmp (str, "TRAFFIC") == 0) || (strcmp (str, "traffic") == 0))
            {
                sscanf_value = sscanf (input_string, "%s %s", demand->type, demand->name);
                if ((sscanf_value < 2) || (sscanf_value == EOF))
                    err_input (ERR_SCAN, *input_line_number);
                while (str[0] != '}')
                {
                    fgets (input_string, STRING_LENGTH + 1, input_stream);
                    (*input_line_number)++;
                    sscanf_value = sscanf (input_string, "%s", str);
                    if ((sscanf_value == 0) || (sscanf_value == EOF))
                        strcpy (str, input_string);
                    if (line_read == 0)
                        if (str[0] == '}')
                            return (END_OF_DEMAND_BLOCK);
                    if (is_comment (str) != YES_PARSER)
                        /* BLOCK IS A DEMAND BLOCK */
                    {
                        line_read++;


                        /**********************************************************************/
                        /**********************************************************************/
                        /**********************************************************************/
                        /* IN CASE NEW ELEMENTS ARE ADDED THEY MUST BE READ IN THIS IF        */
                        /**********************************************************************/
                        /**********************************************************************/
                        /**********************************************************************/


                        if ((strncmp (str, "SOURCE", 6) == 0) || (strncmp (str, "scource", 6) == 0))
                            /* READ SOURCE */
                        {
                            read_fields++;
                            sscanf_value = sscanf (input_string, "%s %s", str, (demand->source));
                            if (demand->source[strlen(demand->source)-1] == ';')
                                demand->source[strlen(demand->source)-1] = '\0';
                            if ((sscanf_value < 2) || (sscanf_value == EOF))
                                err_input (ERR_SCAN, *input_line_number);
                            if ((is_valid_source (demand->source)) != OK_PARSER)
                                err_input (ERR_SOURCE, *input_line_number);
                        }

                        if ((strncmp (str, "DESTINATION", 6) == 0) || (strncmp (str, "destination", 6) == 0))
                            /* READ DESTINATION */
                        {
                            read_fields++;
                            sscanf_value = sscanf (input_string, "%s %s", str, (demand->destination));
                            if (demand->destination[strlen(demand->destination)-1] == ';')
                                demand->destination[strlen(demand->destination)-1] = '\0';
                            if ((sscanf_value < 2) || (sscanf_value == EOF))
                                err_input (ERR_SCAN, *input_line_number);
                            if ((is_valid_destination (demand->destination)) != OK_PARSER)
                                err_input (ERR_DESTINATION, *input_line_number);
                        }

                        if ((strcmp (str, "DEMAND") == 0) || (strcmp (str, "demand") == 0))
                            /* READ DEMAND AMOUNT */
                        {
                            read_fields++;
                            sscanf_value = sscanf (input_string, "%s %f", str, &(demand->demand));
                            if ((sscanf_value < 2) || (sscanf_value == EOF))
                                err_input (ERR_SCAN, *input_line_number);
                            if ((is_valid_demand (demand->demand)) != OK_PARSER)
                                err_input (ERR_DEMAND, *input_line_number);
                        }

                        if ((strncmp (str, "DEMAND_TYPE", 11) == 0) ||
                                (strncmp (str, "demand_type", 11) == 0))
                            /* READ DEMAND_TYPE */
                        {
                            read_fields++;
                            sscanf_value = sscanf (input_string, "%s %s", str, (demand->demand_type));
                            if ((sscanf_value < 2) || (sscanf_value == EOF))
                                err_input (ERR_SCAN, *input_line_number);
                            if ((is_valid_demand_type (demand->demand_type)) != OK_PARSER)
                                err_input (ERR_DEMAND_TYPE, *input_line_number);
                        }

                        if ((strncmp (str, "CLASS", 5) == 0) ||
                                (strncmp (str, "class", 5) == 0))
                            /* READ CLASS */
                        {
                            read_fields++;
                            sscanf_value = sscanf (input_string, "%s %d", str, &(demand->demand_class));
                            if ((sscanf_value < 2) || (sscanf_value == EOF))
                                err_input (ERR_SCAN, *input_line_number);
                            if ((is_valid_class (demand->demand_class)) != OK_PARSER)
                                err_input (ERR_CLASS, *input_line_number);
                        }


                        /************************* ADD READ LIGHTPATHS !!!**************/
                    }
                }
            }
        }
    }
    if (read_fields <= 0)
        err_input (NO_FIELDS, *input_line_number);
    return (OK_PARSER);
}

/***************************************************************************/
/* 05/05/01                                                                */
/* FUNCTION GET_DEMAND_SOURCE                                              */
/* RETURNS THE DEMAND SOURCE OR OR NULL ON INVALID TYPE                    */
/* THE MEMORY NECESSARY TO HOLD THE NAME IS ALLOCATED HERE                 */
/***************************************************************************/
char *
get_demand_source (struct demand_block demand)
{
    char *source;
    int n;
    n = strlen (demand.source);
    if (n > 0)
        if (demand.source[n - 1] == ';')
            demand.source[n - 1] = '\0';
    source = (char *) malloc (n * sizeof (char));
    strcpy (source, demand.source);
    return (source);
}

/***************************************************************************/
/* 05/05/01                                                                */
/* FUNCTION GET_DEMAND_DESTINATION                                         */
/* RETURNS THE DEMAND DESTINATION OR OR NULL ON INVALID TYPE               */
/* THE MEMORY NECESSARY TO HOLD THE NAME IS ALLOCATED HERE                 */
/***************************************************************************/
char *
get_demand_destination (struct demand_block demand)
{
    char *dest;
    int n;
    n = strlen (demand.destination);
    if (n > 0)
        if (demand.destination[n - 1] == ';')
            demand.destination[n - 1] = '\0';
    dest = (char *) malloc (n * sizeof (char));
    strcpy (dest, demand.destination);
    return (dest);
}


/***************************************************************************/
/* 05/05/01                                                                */
/* FUNCTION GET_DEMAND_DEMAND                                              */
/* RETURNS THE DEMAND AMOUNT OR MAXFLOAT ON ERROR                          */
/***************************************************************************/
float
get_demand_demand (struct demand_block demand)
{
    if (is_valid_demand (demand.demand) == OK_PARSER)
        return (demand.demand);
    return (MAXFLOAT);
}

/***************************************************************************/
/* 05/05/01                                                                */
/* FUNCTION GET_DEMAND_TYPE                                                */
/* RETURNS THE DEMAND TYPE OR NULL ON INVALID TYPE                         */
/* THE MEMORY NECESSARY TO HOLD THE NAME IS ALLOCATED HERE                 */
/***************************************************************************/
char *
get_demand_type (struct demand_block demand)
{
    char *type;
    int n;
    n = strlen (demand.demand_type);
    if (n > 0)
        if (demand.demand_type[n - 1] == ';')
            demand.demand_type[n - 1] = '\0';
    type = (char *) malloc (n * sizeof (char));
    strcpy (type, demand.demand_type);
    return (type);
}
/***************************************************************************/
/* 05/05/01                                                                */
/* FUNCTION GET_DEMAND_CLASS                                               */
/* RETURNS THE DEMAND CLASS OR MAXFLOAT ON ERROR                           */
/***************************************************************************/
int
get_demand_class (struct demand_block demand)
{
    if (is_valid_class (demand.demand_class) == OK_PARSER)
        return (demand.demand_class);
    return (MAXINT);
}

/***************************************************************************/
/* 06/19/01                                                                */
/* FUNCTION FREEDATA                                                       */
/* FREE THE MEMORY OF A NODE BLOCK                                         */
/***************************************************************************/
void freeData (struct node_block& node)
{
    // the following fields have not been dynamically allocated
    //  free (node.type);
    //  free (node.name);
    //  free (node.op_type);
    //  free (node.status);
}

/***************************************************************************/
/* 06/19/01                                                                */
/* FUNCTION FREEDATA                                                       */
/* FREE THE MEMORY OF A LINK BLOCK                                         */
/***************************************************************************/
void freeData (struct link_block& link)
{
    //  free (link.type);
    //  free (link.name);
    //  free (link.from);
    //  free (link.to);
    for (int i = 0; i < link.fiber_ratio; i++)
    {
        free (link.fiber_array[i].type);
        free (link.fiber_array[i].name);
        free (link.fiber_array[i].status);
        free (link.fiber_array[i].op_type);
        free_lambda_array (link.fiber_array[i].lambda_array);
        free_amply_array (link.fiber_array[i].amply_array);
        link.fiber_array[i].lambda_array = NULL;
        link.fiber_array[i].amply_array = NULL;
    }
    free (link.fiber_array);
}


/***************************************************************************/
/* 06/19/01                                                                */
/* FUNCTION FREEDATA                                                       */
/* FREE THE MEMORY OF A DEMAND BLOCK                                       */
/***************************************************************************/
void freeData (struct demand_block& demand)
{
    // the following fields have not been dynamically allocated
    // free(demand.name);
    //  free(demand.type);
    //  free(demand.source);
    //  free(demand.destination);
    //  free(demand.demand_type);
}

