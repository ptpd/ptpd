/** 
 * @file OptBuffer.h
 * @author Tomasz Kleinschmidt
 *
 * @brief OptBuffer class definition
 */

#ifndef OPTBUFFER_H
#define	OPTBUFFER_H

class OptBuffer {
public:
    OptBuffer(char* appName);
    virtual ~OptBuffer();
    
    char* help_arg;
    char* u_address;
    char* u_port;
    
    bool msg_print;
    bool help_print;

private:
    
};

#endif	/* OPTBUFFER_H */

