#ifndef PARSER_H
#define PARSER_H

#include "defs.h"
#include "parser_data.h"
#include "parser_hidden_structures.h"


using namespace std;
/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTION IS_COMMENT: RETURN YES_PARSER IF LINE IS A COMMENT              */
/****************************************************************************/
int is_comment(char str[]);

/****************************************************************************/
/* 04/26/01                                                                 */
/* FUNCTION GET_NEXT_LINE: READ A LINE FROM INPT FILE AND RETURNS A CODE    */
/* IDENTIFYING THE LINE                                                     */
/****************************************************************************/
int get_next_line(FILE *input_stream,int *pline_number);

/****************************************************************************/
/* 04/25/01                                                                 */
/* FUNCTION FREE_LAMBDA_ARRAY                                               */
/****************************************************************************/
void free_lambda_array(struct lambda_elem *array);

/****************************************************************************/
/* 04/25/01                                                                 */
/* FUNCTION FREE_AMPLY_ARRAY                                                */
/****************************************************************************/
void free_amply_array(struct amplifier_elem *array);

/***************************************************************************/
/* 03/04/10                                                             */
/* FUNCTION INIT_DOMAIN: INITIALIZE DOMAIN VALUES                       */
/* By Cialo @ SSSUP 							*/
/***************************************************************************/
int init_domain (struct domain_block *pdomain);

/***************************************************************************/
/* 04/25/01                                                                */
/* FUNCTION INIT_NODE: INITIALIZE NODE VALUES                              */
/***************************************************************************/
int init_node(struct node_block *pnode);

/***************************************************************************/
/* 04/25/01                                                                */
/* FUNCTION INIT_LINK_MAIN: INITIALIZE LINK VALUES TO BE USED IN THE MAIN  */
/***************************************************************************/
int init_link_main(struct link_block *plink);

/***************************************************************************/
/* 04/25/01                                                                */
/* FUNCTION FREE_LINK: INITIALIZE LINK VALUES AND FREE LINK ALLOCATED MEM  */
/***************************************************************************/
int free_link(struct link_block *plink);

/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTION GET_DECISION                                                    */
/****************************************************************************/
int get_decision();

/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTION ERR_INPUT MANAGES ERRORS FROM PARSING THE INPUT FILE            */
/****************************************************************************/
int err_input(int code,int line);

/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTIONS IS_VALID_XPOS RETURNS OK_PARSER IF XOPSITION IS A VALID RANGE  */
/****************************************************************************/
int is_valid_xpos(float xpos);

/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTIONS IS_VALID_nodeID RETURNS OK IF XOPSITION IS A VALID RANGE       */
/****************************************************************************/
int is_valid_nodeID(int ID);

/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTIONS IS_VALID_BER RETURNS OK IF BER IS IN A VALID RANGE             */
/****************************************************************************/
int is_valid_ber(float ber);

/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTIONS IS_VALID_REC_SENSITIVITY RETURNS OK_PARSER IF REC_SENSITIVITY  */
/* IS A VALID RANGE                                                         */
/****************************************************************************/
int is_valid_rec_sensitivity(float rec_sensitivity);

/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTIONS IS_VALID_TRX_BITRATE RETURNS OK_PARSER IF TRX_BITRATE          */
/* IS A VALID RANGE                                                         */
/****************************************************************************/
int is_valid_trx_bitrate(float trx_bitrate);

/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTIONS IS_VALID_DELAY RETURNS OK_PARSER IF DELAY                      */
/* IS A VALID RANGE                                                         */
/****************************************************************************/
int is_valid_delay(float delay);

/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTIONS IS_VALID_JITTER RETURNS OK_PARSER IF JITTER                    */
/* IS A VALID RANGE                                                         */
/****************************************************************************/
int is_valid_jitter(float jitter);

/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTIONS IS_VALID_BW RETURNS OK_PARSER IF BANDWIDTH                     */
/* IS A VALID RANGE                                                         */
/****************************************************************************/
int is_valid_bw(float bw);

/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTIONS IS_VALID_OP_TYPE RETURNS OK_PARSER IF OP_TYPE                  */
/* IS A VALID                                                               */
/****************************************************************************/
int is_valid_op_type(char *op_type);

/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTIONS IS_VALID_STATUS RETURNS OK_PARSER IF STATUS                    */
/* IS A VALID                                                               */
/****************************************************************************/
int is_valid_status(char *status);

/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTIONS IS_VALID_MTBF RETURNS OK_PARSER IF MTBF                        */
/* IS A VALID RANGE                                                         */
/****************************************************************************/
int is_valid_mtbf(float mtbf);

/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTIONS IS_VALID_MTTR RETURNS OK_PARSER IF MTTR                        */
/* IS A VALID RANGE                                                         */
/****************************************************************************/
int is_valid_mttr(float mttr);

/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTIONS IS_VALID_LINKID RETURNS OK_PARSER IF LINKID                    */
/* IS A VALID RANGE                                                         */
/****************************************************************************/
int is_valid_linkID(int linkID);

/****************************************************************************/
/* 11/20/02                                                                 */
/* FUNCTIONS IS_VALID_LINK_MAXBYTE RETURNS OK_PARSER IF LINKID                    */
/* IS A VALID RANGE                                                         */
/****************************************************************************/
int is_valid_link_maxbyte(int maxbyte);

/****************************************************************************/
/* 11/20/02                                                                 */
/* FUNCTIONS IS_VALID_LINK_CHANNEL RETURNS OK_PARSER IF LINKID                    */
/* IS A VALID RANGE                                                         */
/****************************************************************************/
int is_valid_link_num_channel(int maxbyte);

/****************************************************************************/
/* 12/03/02                                                                 */
/* FUNCTIONS IS_VALID_LINK_BITRATE RETURNS OK_PARSER IF LINKID                    */
/* IS A VALID RANGE                                                         */
/****************************************************************************/
int is_valid_link_bitrate(int bitrate);

/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTIONS IS_VALID_LINK_DISTANCE RETURNS OK_PARSER IF DISTANCE           */
/* IS A VALID RANGE                                                         */
/****************************************************************************/
int is_valid_link_distance(float distance);

/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTIONS IS_VALID_LINK_BER RETURNS OK_PARSER IF BER                     */
/* IS A VALID RANGE                                                         */
/****************************************************************************/
int is_valid_link_ber(float ber);

/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTIONS IS_VALID_LINK_DELAY RETURNS OK_PARSER IF DELAY                 */
/* IS A VALID RANGE                                                         */
/****************************************************************************/
int is_valid_link_delay(float delay);

/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTIONS IS_VALID_LINK_JITTER RETURNS OK_PARSER IF JITTER               */
/* IS A VALID RANGE                                                         */
/****************************************************************************/
int is_valid_link_jitter(float jitter);

/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTIONS IS_VALID_LINK_BANDWIDTH RETURNS OK_PARSER IF BANDWIDTH         */
/* IS A VALID RANGE                                                         */
/****************************************************************************/
int is_valid_link_bandwidth(float bw);

/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTIONS IS_VALID_LINK_MFP RETURNS OK_PARSER IF MFP                     */
/* IS A VALID RANGE                                                         */
/****************************************************************************/
int is_valid_link_mfp(float mfp);

/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTIONS IS_VALID_LINK_MTBF RETURNS OK_PARSER IF MTBF                   */
/* IS A VALID RANGE                                                         */
/****************************************************************************/
int is_valid_link_mtbf(float mtbf);

/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTIONS IS_VALID_LINK_MTTR RETURNS OK_PARSER IF MTTR                   */
/* IS A VALID RANGE                                                         */
/****************************************************************************/
int is_valid_link_mttr(float mttr);

/****************************************************************************/
/* 03/28/01                                                                 */
/* FUNCTIONS IS_VALID_LINK_FIBER_RATIO RETURNS OK_PARSER IF FIBER_RATIO     */
/* IS A VALID RANGE                                                         */
/****************************************************************************/
int is_valid_link_fiber_ratio(int fiber_ratio);

/****************************************************************************/
/* 04/18/01                                                                 */
/* FUNCTIONS IS_VALID_FIBER_LIST_ID RETURNS OK_PARSER IF ID                 */
/* IS IN A VALID RANGE                                                      */
/****************************************************************************/
int is_valid_fiber_list_ID(int ID);

/****************************************************************************/
/* 04/18/01                                                                 */
/* FUNCTIONS IS_VALID_LIST_OP_TYPE RETURNS OK_PARSER IF OP_TYPE             */
/* IS IN A VALID RANGE                                                      */
/****************************************************************************/
int is_valid_fiber_list_op_type(char *op_type);

/****************************************************************************/
/* 04/18/01                                                                 */
/* FUNCTIONS IS_VALID_FIBER_LIST_STATUS RETURNS OK_PARSER IF STATUS         */
/* IS IN A VALID RANGE                                                      */
/****************************************************************************/
int is_valid_fiber_list_status(char *status);

/****************************************************************************/
/* 04/18/01                                                                 */
/* FUNCTIONS IS_VALID_FIBER_LIST_LAMBDA_RATIO RETURNS OK_PARSER IF          */
/* LAMBDA_RATIO                                                             */
/* IS IN A VALID RANGE                                                      */
/****************************************************************************/
int is_valid_fiber_list_lambda_ratio(int lambda_ratio);

/****************************************************************************/
/* 04/26/01                                                                 */
/* FUNCTION IS_VALID_LAMBDA_STATUS                                          */
/****************************************************************************/
int is_valid_lambda_status(char *lambda_status);

/****************************************************************************/
/* 04/26/01                                                                 */
/* FUNCTION IS_VALID_LAMBDA_OP_TYPE                                         */
/****************************************************************************/
int is_valid_lambda_op_type(char *lambda_status);

/****************************************************************************/
/* 04/26/01                                                                 */
/* FUNCTION IS_VALID_LAMBDA_ID                                              */
/****************************************************************************/
int is_valid_lambda_ID(int ID);

/****************************************************************************/
/* 04/18/01                                                                 */
/* FUNCTION IS_VALID_FIBER_AMPLIFIER_RATIO RETURNS OK_PARSER IF THE         */
/* AMPLIFIER RATIO IS IN A VALID RANGE                                      */
/****************************************************************************/
int is_valid_fiber_amplifier_ratio(int ratio);

/****************************************************************************/
/* 04/26/01                                                                 */
/* FUNCTION IS VALID AMPLY POS RETURN OK_PARSER IF 0 < POS < DISTANCE       */
/****************************************************************************/
int is_valid_amply_pos(float pos,float distance);

/*************************************************************************/
/* 04/18/01                                                              */
/* FUNCTION ALLOCATE_FIBER_LIST_ARRAY ALLOCATES THE ARRAY FOR THE        */
/* FIBER ARRAY                                                           */
/*************************************************************************/
int allocate_fiber_list_array(struct link_block *p_link);

/*************************************************************************/
/* 04/18/01                                                              */
/* FUNCTION ALLOCATE_AMPLIFIER_ARRAY ALLOCATES THE ARRAY NECESSARY TO    */
/* CONTAIN THE INFORMATION ON THE AMPLY_ARRAY ARRAY                      */
/*************************************************************************/
int allocate_amplifier_array(struct fiber_list *elem);

/*************************************************************************/
/* 04/18/01                                                              */
/* FUNCTION ALLOCATE_LAMBDA_LIST ALLOCATES THE ARRAY NECESSARY TO        */
/* CONTAIN THE INFORMATION ON THE LAMBDA_LIST LIST                       */
/*************************************************************************/
int allocate_lambda_list_array(struct fiber_list *elem);

/************************************************************************/
/* 04/18/01                                                             */
/* FUNCTION READ_LAMBDA_LIST_BLOCK READS A LAMBDA BLOCK IN THE FIBER    */
/* LIST                                                                 */
/************************************************************************/
int read_lambda_list_block(FILE *input_stream,struct lambda_elem *elem,
                           int n_lambda,int *input_line_number);

/************************************************************************/
/* 04/18/01                                                             */
/* FUNCTION READ_AMPLIFIER_BLOCK READS AN AMPLY BLOCK IN THE FIBER      */
/* LIST                                                                 */
/************************************************************************/
int read_amplifier_block(FILE *input_stream,struct amplifier_elem *elem,
                         int n_amply,int *input_line_number);

/***************************************************************************/
/* 04/26/01                                                                */
/* FUNCTION IS_FIBER_TYPE RETURNS OK_PARSER IF STR IS A VALID FIBER TYPE   */
/***************************************************************************/
int is_valid_fiber_type(char *str);

/***************************************************************************/
/* 03/23/01                                                                */
/* FUNCTION READ_FIBER_LIST_BLOCK: READ THE FIBER_LIST INTO THE LINK BLOCK */
/***************************************************************************/
int read_fiber_list_block(struct fiber_list *fiber_array,int i,int
                          *input_line_number,FILE *input_stream);

/****************************************************************************/
/* 03/03/10                                                                 */
/* FUNCTION READ_DOMAIN_BLOCK: READS A BLOCK OF DATA AND PUTS IT IN THE     */
/* APPROPRIATE STRUCTURE                                                    */
/* By Cialo & Wamba @ SSSUP 						    */
/****************************************************************************/
int read_domain_block(struct domain_block *node,
                      FILE *input_stream,int *input_line_number);

/****************************************************************************/
/* 03/23/01                                                                 */
/* FUNCTION READ_NODE_BLOCK: READS A BLOCK OF DATA AND PUTS IT IN THE       */
/* APPROPRIATE STRUCTURE                                                    */
/****************************************************************************/
int read_node_block(struct node_block *node,
                    FILE *input_stream,int *input_line_number);

/****************************************************************************/
/* 03/27/01                                                                 */
/* FUNCTION READ_LINK_BLOCK: READ THE LINK BLOCK IN THE FILE                */
/****************************************************************************/
int read_link_block(struct link_block *p_link,FILE *input_stream,
                    int *input_line_number);

/***************************************************************************/
/* 04/25/01                                                                */
/* FUNCTION GET_NODE_XPOS                                                  */
/* RETURNS THE NODE POSITION OR MAXFLOAT ON ERROR                          */
/***************************************************************************/
float get_node_xpos(struct node_block node);

/***************************************************************************/
/* 04/25/01                                                                */
/* FUNCTION GET_NODE_YPOS                                                  */
/* RETURNS THE NODE POSITION OR MAXFLOAT ON ERROR                          */
/***************************************************************************/
float get_node_ypos(struct node_block node);

/***************************************************************************/
/* 04/25/01                                                                */
/* FUNCTION GET_NODE_ID RETURNS NODE ID OR -1 ON INVALID NODE ID           */
/***************************************************************************/
int get_node_ID(struct node_block node);

/**************************************************************************/
/* 04/25/01                                                               */
/* FUNCTION GET_NODE_NAME RETURNS NAME OR NULL POINTER ON INVALID NAME    */
/* THE MEMORY NECESSARY TO HOLD THE NAME IS ALLOCATED HERE                */
/**************************************************************************/
char *get_node_name(struct node_block node);

/***************************************************************************/
/* 04/25/01                                                                */
/* FUNCTION GET_NODE_TYPE RETURNS TYPE OR NULL ON INVALID TYPE             */
/* THE MEMORY NECESSARY TO HOLD THE NAME IS ALLOCATED HERE                 */
/***************************************************************************/
char *get_node_type(struct node_block node);

/****************************************************************************/
/* 04/25/01                                                                 */
/* FUNCTION GET_NODE_TRX_BITRATE RETURNS TRANSMISSION BIT RATE OF THE NODE  */
/* OR -1.0 ON INVALID TRANSMISSION BIT_RATE                                 */
/****************************************************************************/
float get_node_trx_bitrate(struct node_block node);

/****************************************************************************/
/* 04/25/01                                                                 */
/* FUNCTION GET_NODE_REC_SENSITIVITY RETURNS RECEIVER SENSITIVITY           */
/* OR -1.0 ON INVALID VALUE                                                 */
/****************************************************************************/
float get_node_rec_sensitivity(struct node_block node);

/****************************************************************************/
/* 04/25/01                                                                 */
/* FUNCTION GET_NODE_BER RETURNS NODE BER OR -1.0 ON INVALID VALUE          */
/****************************************************************************/
float get_node_ber(struct node_block node);

/****************************************************************************/
/* 04/25/01                                                                 */
/* FUNCTION GET_NODE_DELAY RETURNS NODE DELAY OR -1.0 ON INVALID VALUE      */
/****************************************************************************/
float get_node_delay(struct node_block node);

/****************************************************************************/
/* 04/25/01                                                                 */
/* FUNCTION GET_NODE_JITTER RETURNS NODE JITTER OR -1.0 ON INVALID VALUE    */
/****************************************************************************/
float get_node_jitter(struct node_block node);

/****************************************************************************/
/* 04/25/01                                                                 */
/* FUNCTION GET_NODE_BW RETURNS NODE BW OR -1.0 ON INVALID VALUE            */
/****************************************************************************/
float get_node_bw(struct node_block node);

/****************************************************************************/
/* 04/25/01                                                                 */
/* FUNCTION GET_NODE_MTBF RETURNS NODE MTBF OR -1.0 ON INVALID VALUE        */
/****************************************************************************/
float get_node_mtbf(struct node_block node);

/****************************************************************************/
/* 04/25/01                                                                 */
/* FUNCTION GET_NODE_MTTR RETURNS NODE MTTR OR -1.0 ON INVALID VALUE     */
/****************************************************************************/
float get_node_mttr(struct node_block node);

/****************************************************************************/
/* 04/25/01                                                                 */
/* FUNCTION GET_NODE_OP_TYPE RETURNS NODE OP_TYPE OR NULL ON INVALID VALUE  */
/****************************************************************************/
char *get_node_op_type(struct node_block node);

/****************************************************************************/
/* 04/25/01                                                                 */
/* FUNCTION GET_NODE_STATUS RETURNS NODE STATUS OR NULL ON INVALID VALUE    */
/****************************************************************************/
char *get_node_status(struct node_block node);

/****************************************************************************/
/* 04/26/01                                                                 */
/* FUNCTION GET_LINK_TYPE RETURNS LINK TYPE OR NULL ON INVALID VALUE        */
/****************************************************************************/
char *get_link_type(struct link_block link);

/****************************************************************************/
/* 04/26/01                                                                 */
/* FUNCTION GET_LINK_NAME RETURNS LINK NAME OR NULL ON INVALID VALUE        */
/****************************************************************************/
char *get_link_name(struct link_block link);

/****************************************************************************/
/* 04/26/01                                                                 */
/* FUNCTION GET_LINK_FROM RETURNS THE NODE NAME FROM WHICH THE LINK         */
/* ORIGINATES OR NULL ON INVALID VALUE                                      */
/****************************************************************************/
char *get_link_from(struct link_block link);

/****************************************************************************/
/* 04/26/01                                                                 */
/* FUNCTION GET_LINK_TO RETURNS THE NODE NAME TO WHICH THE LINK             */
/* DESTINATED OR NULL ON INVALID VALUE                                      */
/****************************************************************************/
char *get_link_to(struct link_block link);

/***************************************************************************/
/* 04/26/01                                                                */
/* FUNCTION GET_LINK_ID RETURNS LINK ID OR -1 ON INVALID LINK ID           */
/***************************************************************************/
int get_link_ID(struct link_block link);

/***************************************************************************/
/* 04/26/01                                                                */
/* FUNCTION GET_LINK_DISTANCE RETURNS LINK DISTANCE OR -1.0 ON             */
/* INVALID LINK DISTANCE                                                   */
/***************************************************************************/
float get_link_distance(struct link_block link);

/***************************************************************************/
/* 04/26/01                                                                */
/* FUNCTION GET_LINK_BER RETURNS LINK BER OR -1.0 ON                       */
/* INVALID LINK BER                                                        */
/***************************************************************************/
float get_link_ber(struct link_block link);

/***************************************************************************/
/* 04/26/01                                                                */
/* FUNCTION GET_LINK_DELAY RETURNS LINK DELAY OR -1.0 ON                   */
/* INVALID LINK DELAY                                                      */
/***************************************************************************/
float get_link_delay(struct link_block link);

/***************************************************************************/
/* 04/26/01                                                                */
/* FUNCTION GET_LINK_JITTER RETURNS LINK JITTER OR -1.0 ON                 */
/* INVALID LINK DISTANCE                                                   */
/***************************************************************************/
float get_link_jitter (struct link_block link);

/***************************************************************************/
/* 04/26/01                                                                */
/* FUNCTION GET_LINK_BW RETURNS LINK BANDWIDTH OR -1.0 ON                  */
/* INVALID LINK BW                                                         */
/***************************************************************************/
float get_link_bw(struct link_block link);

/***************************************************************************/
/* 04/26/01                                                                */
/* FUNCTION GET_LINK_MFP RETURNS LINK MFP OR -1.0 ON                       */
/* INVALID LINK MFP                                                        */
/***************************************************************************/
float get_link_mfp(struct link_block link);

/***************************************************************************/
/* 04/26/01                                                                */
/* FUNCTION GET_LINK_MTBF RETURNS LINK MTBF OR -1.0 ON                     */
/* INVALID LINK MTBF                                                       */
/***************************************************************************/
float get_link_mtbf(struct link_block link);

/***************************************************************************/
/* 04/26/01                                                                */
/* FUNCTION GET_LINK_MTTR RETURNS LINK MTTR OR -1.0 ON                     */
/* INVALID LINK MTTR                                                       */
/***************************************************************************/
float get_link_mttr(struct link_block link);

/***************************************************************************/
/* 04/26/01                                                                */
/* FUNCTION GET_LINK_FIBER_RATIO RETURNS LINK FIBER_RATIO OR -1 ON       */
/* INVALID LINK FIBER_RATIO                                                */
/***************************************************************************/
int get_link_fiber_ratio(struct link_block link);

/***************************************************************************/
/* 04/26/01                                                                */
/* FUNCTION GET_LINK_FIBER_NAME RETURNS LINK FIBER NAME OF THE I-TH FIBER  */
/* OR NULL ON INVALID NAME                                                 */
/***************************************************************************/
char *get_link_fiber_name(struct link_block link,int i);

/***************************************************************************/
/* 04/26/01                                                                */
/* FUNCTION GET_LINK_FIBER_TYPE RETURNS LINK FIBER TYPE OF THE I-TH FIBER  */
/* OR NULL ON INVALID TYPE                                                 */
/***************************************************************************/
char *get_link_fiber_type(struct link_block link,int i);

/***************************************************************************/
/* 04/26/01                                                                */
/* FUNCTION GET_LINK_FIBER_STATUS RETURNS FIBER STATUS OF THE I-TH FIBER   */
/* OR NULL ON INVALID NAME                                                 */
/***************************************************************************/
char *get_link_fiber_status(struct link_block link,int i);

/***************************************************************************/
/* 04/26/01                                                                */
/* FUNCTION GET_LINK_FIBER_OP_TYPE RETURNS FIBER OP_TYPE OF THE I-TH FIBER */
/* OR NULL ON INVALID OP_TYPE                                              */
/***************************************************************************/
char *get_link_fiber_op_type(struct link_block link,int i);

/***************************************************************************/
/* 04/26/01                                                                */
/* FUNCTION GET_LINK_FIBER_ID RETURNS FIBER ID OF THE I-TH FIBER           */
/* OR -1 ON INVALID ID                                                     */
/***************************************************************************/
int get_link_fiber_ID(struct link_block link,int i);

/***************************************************************************/
/* 04/26/01                                                                */
/* FUNCTION GET_LINK_FIBER_LAMBDA_RATIO RETURNS FIBER LAMBDA_RATIO         */
/*  OF THE I-TH FIBER OR -1 ON INVALID LAMBDA_RATIO                        */
/***************************************************************************/
int get_link_fiber_lambda_ratio(struct link_block link,int i);

/***************************************************************************/
/* 04/26/01                                                                */
/* FUNCTION GET_LINK_FIBER_AMPLY_RATIO RETURNS FIBER AMPLY_RATIO           */
/*  OF THE I-TH FIBER OR -1 ON INVALID AMPLY_RATIO                         */
/***************************************************************************/
int get_link_fiber_amply_ratio(struct link_block link,int i);

/*****************************************************************************/
/* 04/26/01                                                                  */
/* FUNCTION GET_LINK_FIBER_LAMBDA_NAME RETURNS LAMBDA NAME OF THE J-TH       */
/* LAMBDA ON THE I-TH FIBER OR NULL ON INVALID VALUE                         */
/*****************************************************************************/
char *get_link_fiber_lambda_name(struct link_block link,int i,int j);

/*****************************************************************************/
/* 04/26/01                                                                  */
/* FUNCTION GET_LINK_FIBER_LAMBDA_OP_TYPE RETURNS LAMBDA OP_TYPE OF THE J-TH */
/* LAMBDA ON THE I-TH FIBER OR NULL ON INVALID OP_TYPE                       */
/*****************************************************************************/
char *get_link_fiber_lambda_op_type(struct link_block link,int i,int j);

/*****************************************************************************/
/* 04/26/01                                                                  */
/* FUNCTION GET_LINK_FIBER_LAMBDA_STATUS RETURNS LAMBDA STATUS OF THE J-TH   */
/* LAMBDA ON THE I-TH FIBER OR NULL ON INVALID STATUS                        */
/*****************************************************************************/
char *get_link_fiber_lambda_status(struct link_block link,int i,int j);

/*****************************************************************************/
/* 04/26/01                                                                  */
/* FUNCTION GET_LINK_FIBER_LAMBDA_ID RETURNS LAMBDA ID OF THE J-TH           */
/* LAMBDA ON THE I-TH FIBER OR -1 ON INVALID ID                              */
/*****************************************************************************/
int get_link_fiber_lambda_ID(struct link_block link,int i,int j);

/*****************************************************************************/
/* 04/26/01                                                                  */
/* FUNCTION GET_LINK_AMPLY_POS RETURNS POSITION OF THE J-TH                  */
/* AMPLIFIER ON THE I-TH FIBER OR -1.0 ON INVALID ID                         */
/*****************************************************************************/
float get_link_fiber_amply_pos(struct link_block link,int i,int j);

/*****************************************************************************/
/* 04/26/01                                                                  */
/* FUNCTION GET_LINK_AMPLY_TYPE RETURNS TYPE OF THE J-TH                     */
/* AMPLIFIER ON THE I-TH FIBER OR NULL ON INVALID TYPE                       */
/*****************************************************************************/
char *get_link_fiber_amply_type(struct link_block link,int i,int j);

/*****************************************************************************/
/* 04/26/01                                                                  */
/* FUNCTION GET_LINK_AMPLY_NAME RETURNS NAME OF THE J-TH                     */
/* AMPLIFIER ON THE I-TH FIBER OR NULL ON INVALID NAME                       */
/*****************************************************************************/
char *get_link_fiber_amply_name(struct link_block link,int i,int j);

/*****************************************************************************/
/* 05/26/01                                                                  */
/* FUNCTION READING THE DEMAND BLOCK                                         */
/*****************************************************************************/
int read_demand_block (struct demand_block *demand, FILE * input_stream, int *input_line_number);


/*****************************************************************************/
/* 05/26/01                                                                  */
/* FUNCTION READING THE INPUT FILE                                           */
/*****************************************************************************/
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
);

/***************************************************************************/
/* 06/19/01                                                                */
/* FUNCTION FREEDATA                                                       */
/* FREE THE MEMORY OF A NODE BLOCK                                         */
/***************************************************************************/
void freeData (struct node_block& node);

/***************************************************************************/
/* 06/19/01                                                                */
/* FUNCTION FREEDATA                                                       */
/* FREE THE MEMORY OF A LINK BLOCK                                         */
/***************************************************************************/
void freeData (struct demand_block& demand);

/***************************************************************************/
/* 06/19/01                                                                */
/* FUNCTION FREEDATA                                                       */
/* FREE THE MEMORY OF A DEMAND BLOCK                                       */
/***************************************************************************/
void freeData (struct link_block& link);

#endif
