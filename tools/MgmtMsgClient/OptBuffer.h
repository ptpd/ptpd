/** 
 * @file        OptBuffer.h
 * @author      Tomasz Kleinschmidt
 *
 * @brief       OptBuffer class definition.
 */

#ifndef OPTBUFFER_H
#define	OPTBUFFER_H

class OptBuffer {
public:
    OptBuffer(char* appName);
    virtual ~OptBuffer();
    
    bool action_type_set;
    bool help_print;
    bool interface_set;
    bool mgmt_id_set;
    bool msg_print;
    bool value_set;
    
    char* help_arg;
    char* interface;
    char* u_address;
    char* u_port;
    char* value;
    
    unsigned int action_type;
    unsigned int mgmt_id;
    unsigned int timeout;
    
    void mgmtActionTypeParser(char* actionType);
    void mgmtIdParser(char* mgmtId);

private:
};

#endif	/* OPTBUFFER_H */

