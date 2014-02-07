
//-------------------------------------------------------------------------------------------------------------------------//
//                   Max object fhnw.audiowalk.state                                                                       //
//                   Author Thomas Resch FHNW, University of Music, Electronic Studio & Research and Development           //
//                   State Machine for Position Based Audiowalks                                                           //
//                                                                                                                         //
//                                                                                                                         //
//                   Type checking in the nextState method is not perfect..                                                //
//                   Other stuff simply not finished..                                                                     //
//                                                                                                                         //
//                                                                                                                         //
//-------------------------------------------------------------------------------------------------------------------------//

#include "ext.h"
#include "ext_obex.h"
#include "ext_common.h"
#include "jpatcher_api.h"
#include "jgraphics.h"
#include "math.h"
#include <stdlib.h>

#define MAXNUMBEROFPERSONS 32
#define MAXNUMBEROFSTATES 256
#define MAXNUMBEROFFLAGS 3000
#define MAXNUMBEROFGROUPS 8

#define EVENTCONDITION_CLIENTID 1 << 0
#define EVENTCONDITION_STATE 1 << 1
#define EVENTCONDITION_POSITION 1 << 2
#define EVENTCONDITION_ORIENTATION 1 << 3
#define EVENTCONDITION_YES 1 << 4
#define EVENTCONDITION_NO 1 << 5
#define EVENTCONDITION_COMPLY 1 << 6
#define EVENTCONDITION_NOCOMPLY 1 << 7
#define EVENTCONDITION_CLOSEBY 1 << 8
#define EVENTCONDITION_NOTORIENTATIONCHANGED 1 << 9
#define EVENTCONDITION_ELAPSEDTIMEINCURRENTSTATE 1 << 10
#define EVENTCONDITION_ELAPSEDOVERALLTIME 1 << 11
#define EVENTCONDITION_NOTPOSITIONCHANGED 1 << 12
#define EVENTCONDITION_NUMBEROFSTEPS 1 << 13
#define EVENTCONDITION_ANYMESSAGE 1 << 14
#define EVENTCONDITION_NUMBEROFYES 1 << 15
#define EVENTCONDITION_NUMBEROFNO 1 << 16
#define EVENTCONDITION_NUMBEROFCOMPLY 1 << 17
#define EVENTCONDITION_NOFLAGS 1 << 18
#define EVENTCONDITION_FLAGS 1 << 19
#define EVENTCONDITION_ELAPSEDTIMEINCURRENTSTATE_TIMEOUT 1 << 20
#define EVENTCONDITION_NUMBEROFTIMEOUT 1 << 21
#define EVENTCONDITION_NOTPOSITION 1 << 22
#define EVENTCONDITION_POSITIONCHANGED 1 << 23
#define EVENTCONDITION_NOTORIENTATION 1 << 24
#define EVENTCONDITION_NUMBEROFSTEPSINCURRENTSTATE 1 << 25
#define EVENTCONDITION_GETUP 1 << 26
#define EVENTCONDITION_SITDOWN 1 << 27
#define EVENTCONDITION_ORIENTATIONCHANGED 1 << 28
#define EVENTCONDITION_WALKINGDIRECTION 1 << 29
#define EVENTCONDITION_TIMERANGEINCURRENTSTATE 1 << 30
#define EVENTCONDITION_ROTATION 1 << 31

void *fhnwAudiowalkState_class;

typedef struct _fhnwAudiowalkState_condition
{
    long type;
    t_atom values[20];
    
} fhnwAudiowalkState_condition;

typedef struct _fhnwAudiowalkState_message
{
    t_symbol *target;
    t_symbol *message;
    
} fhnwAudiowalkState_message;

typedef struct _fhnwAudiowalkState_event
{
    double wait;
    
    long ID;
    long nextState;
    long flag2set;
    long flag2unset;
    t_linklist *conditions;
    t_linklist *messages;
    
} fhnwAudiowalkState_event;

typedef struct _fhnwAudiowalkState_client
{
    t_symbol *name;
    t_symbol *currentYesOrNo;
    t_symbol *lastYesOrNo;
    t_symbol *currentComplyOrNoComply;
    t_symbol *lastComplyOrNoComply;
    
    t_pt currentPosition;
    t_pt lastPosition;
    t_pt currentPositionForDirection;
    t_pt lastPositionForDirection;
    long walkingDirection;
    
    int active;
    int inWaitMode;
    
    long ID;
    long group;
    long currentState;
    long rotation;
    long lastState;
    long flags[MAXNUMBEROFFLAGS];
    long numberOfYes;
    long numberOfNo;
    long numberOfComply;
    long numberOfNoComply;
    long numberOfTimeOut;
    long room;
    long currentOrientation;
    long lastOrientation;
    long currentNumberOfSteps;
    long lastNumberOfSteps;
    long stride;
    
    double startTimeInCurrentState;
    double waitTimeInCurrentState;
    double startOverallTime;
    
} fhnwAudiowalkState_client;

typedef struct _fhnwAudiowalkState
{
	t_object ob;
    t_clock *timer;
    t_linklist *clientList;
    t_linklist *eventList;
    
    void *outlet;

    int onOff;
    
    long eventIDCounter;
    long clientIDCounter;
    long numberOfClients;
    long numberOfEvents;
    
    double overallTime;
    double groupTime[MAXNUMBEROFGROUPS];
    
    char debuggingMessages2MaxWindow;
    long schedulerInterval;
    
} t_fhnwAudiowalkState;

void *fhnwAudiowalkState_new(t_symbol *s, long argc, t_atom *argv);
void fhnwAudiowalkState_free(t_fhnwAudiowalkState *x);
void fhnwAudiowalkState_assist(t_fhnwAudiowalkState *x, void *b, long m, long a, char *s);
void fhnwAudiowalkState_newClient(t_fhnwAudiowalkState *x, t_symbol *message, short argc, t_atom *argv);
void fhnwAudiowalkState_removeClient(t_fhnwAudiowalkState *x, t_symbol *message, short argc, t_atom *argv);
void fhnwAudiowalkState_nextState(t_fhnwAudiowalkState *x, t_symbol *message, short argc, t_atom *argv);
void fhnwAudiowalkState_removeEvent(t_fhnwAudiowalkState *x, t_symbol *message, short argc, t_atom *argv);
void fhnwAudiowalkState_processEvent(t_fhnwAudiowalkState *x, t_symbol *message, short argc, t_atom *argv);
void fhnwAudiowalkState_read(t_fhnwAudiowalkState *x, t_symbol *message, short argc, t_atom *argv);
void fhnwAudiowalkState_write(t_fhnwAudiowalkState *x, t_symbol *message, short argc, t_atom *argv);
void fhnwAudiowalkState_reset(t_fhnwAudiowalkState *x, t_symbol *message, short argc, t_atom *argv);
void fhnwAudiowalkState_dump(t_fhnwAudiowalkState *x, t_symbol *message, short argc, t_atom *argv);

//--------------------------------------------------------------------------------------------------------------------------------------//
//----------------------- Utility functions --------------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------------------------//

int fhnwAudiowalkState_utility_pointWithinRect(t_pt pt, t_rect rt)
{
	if(pt.x >= rt.x && pt.x < rt.x+rt.width && pt.y >= rt.y && pt.y < rt.y +rt.height)
		return 1;
	else
		return 0;
}

int fhnwAudiowalkState_isWalkingForward(long orientation, long walkingDirection)
{
	if( (abs(orientation-walkingDirection) < 60) || (abs(orientation+360-walkingDirection) < 60))
        return 1;
    else
        return 0;
}

int fhnwAudiowalkState_utility_withinDistance(t_fhnwAudiowalkState *x, t_symbol *message, long argc, t_atom *argv, fhnwAudiowalkState_client *p)
{
    fhnwAudiowalkState_client *currentClient, *nextClient;
    t_symbol *closeByName = NULL;
    float closeByDistance = -1;
	float D = 0;
    closeByName = atom_getsym(argv);
    closeByDistance = atom_getfloat(argv+1);
    
    
    currentClient = linklist_getindex(x->clientList, 0);
    
    while(currentClient != NULL)
    {
        linklist_next(x->clientList, (void *)currentClient, (void *)&nextClient);
        if(currentClient->name != p->name && (currentClient->name == closeByName || closeByName == gensym("anyone")))
        {
            D = sqrt( (pow((p->currentPosition.x - currentClient->currentPosition.x), 2)) + (pow((p->currentPosition.y - currentClient->currentPosition.y), 2)) );
            if(D <= closeByDistance)
                return 1;
        }
        currentClient = nextClient;
        
    }
    return 0;
}

//--------------------------------------------------------------------------------------------------------------------------------------//
//----------------------- fhnwAudiowalkState_tick --------------------------------------------------------------------------------------//
//----------------------- Releases client from waiting and processes events by calling fhnwAudiowalkState_processEvent -----------------//
//----------------------- Called by the clock "timer" every "timerInterval" seconds ----------------------------------------------------//
//----------------------- Also calculates walking direction ----------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------------------------//

void fhnwAudiowalkState_tick(t_fhnwAudiowalkState *x)
{
    fhnwAudiowalkState_client *currentClient, *nextClient;
    float direction = 0;
    x->overallTime+=(x->schedulerInterval/1000.);
    
    currentClient = linklist_getindex(x->clientList, 0);
    while(currentClient != NULL)
    {
        currentClient->lastPositionForDirection.x = currentClient->currentPositionForDirection.x;
        currentClient->lastPositionForDirection.y = currentClient->currentPositionForDirection.y;
        currentClient->currentPositionForDirection.x = currentClient->currentPosition.x;
        currentClient->currentPositionForDirection.y = currentClient->currentPosition.y;
        
        direction = atan2f(currentClient->currentPositionForDirection.x - currentClient->lastPositionForDirection.x, currentClient->lastPositionForDirection.y- currentClient->currentPositionForDirection.y);
        direction *= (180/3.14159265359);
        
        currentClient->walkingDirection = direction;
        currentClient->walkingDirection = ( currentClient->walkingDirection+360) % 360;
        
        if(currentClient->inWaitMode)
        {
            if( (x->overallTime - currentClient->startTimeInCurrentState) >= currentClient->waitTimeInCurrentState)
            {
                currentClient->startTimeInCurrentState = x->overallTime;
                currentClient->waitTimeInCurrentState = 0;
                currentClient->inWaitMode = 0;
            }
        }
        
        linklist_next(x->clientList, (void *)currentClient, (void *)&nextClient);
        currentClient = nextClient;
    }
    
    fhnwAudiowalkState_processEvent(x, 0L, 0, NULL);
    clock_fdelay(x->timer, x->schedulerInterval);
}

//--------------------------------------------------------------------------------------------------------------------------------------//
//----------------------- fhnwAudiowalkState_new ---------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------------------------//

void *fhnwAudiowalkState_new(t_symbol *s, long argc, t_atom *argv)
{
	t_fhnwAudiowalkState *x = NULL;
    
	if ((x = (t_fhnwAudiowalkState *)object_alloc(fhnwAudiowalkState_class)))
    {
        x->timer = clock_new(x,(method)fhnwAudiowalkState_tick); // make a clock
        x->schedulerInterval = 250;
        x->overallTime = 0;
        x->clientList = (t_linklist *)linklist_new();
        linklist_flags(x->clientList, OBJ_FLAG_MEMORY);
        x->eventList = (t_linklist *)linklist_new();
        linklist_flags(x->eventList, OBJ_FLAG_MEMORY);
        x->clientIDCounter = 1;
        x->eventIDCounter = 1;
        x->numberOfClients = 0;
        x->numberOfEvents = 0;
        x->onOff = 0;
        x->outlet = listout((t_object *)x);
    }
    return x;
}

void fhnwAudiowalkState_assist(t_fhnwAudiowalkState *x, void *b, long m, long a, char *s)
{
	if (m == ASSIST_INLET) { // inlet
		sprintf(s, "Inlet");
	}
	else {	// outlet
		sprintf(s, "Outlet");
	}
}

//--------------------------------------------------------------------------------------------------------------------------------------//
//----------------------- fhnwAudiowalkState_reset -------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------------------------//

void fhnwAudiowalkState_reset(t_fhnwAudiowalkState *x, t_symbol *message, short argc, t_atom *argv)
{
    int i = 0;
    fhnwAudiowalkState_client *currentClient, *nextClient;
    t_symbol *clientName = NULL;
    char ID2oSym[10];
 
    if(!argc)
    {
        clock_unset(x->timer);
        x->onOff = 0;
        x->overallTime = 0;
        post("Resetting all clients");
        post("Stopped Scheduler");
        
        currentClient = linklist_getindex(x->clientList, 0);
        while(currentClient != NULL)
        {
            linklist_next(x->clientList, (void *)currentClient, (void *)&nextClient);
            currentClient->inWaitMode = 0;
            currentClient->currentState = 0;
            currentClient->lastState = -1;
            currentClient->numberOfNo = 0;
            currentClient->numberOfYes = 0;
            currentClient->numberOfComply = 0;
            currentClient->numberOfNoComply = 0;
            currentClient->numberOfTimeOut = 0;
            currentClient->currentPosition.x = -1;
            currentClient->currentPosition.y = -1;
            currentClient->lastPosition.x = -1;
            currentClient->lastPosition.y = -1;
            currentClient->currentPositionForDirection.x = -1;
            currentClient->currentPositionForDirection.y = -1;
            currentClient->lastPositionForDirection.x = -1;
            currentClient->lastPositionForDirection.y = -1;
            currentClient->walkingDirection = 0;
            currentClient->lastOrientation = -1;
            currentClient->currentOrientation = -1;
            currentClient->lastYesOrNo = 0;
            currentClient->rotation = 0;
            currentClient->startOverallTime = -1;
            currentClient->startTimeInCurrentState = 0;
            currentClient->waitTimeInCurrentState = 0;
            for(i=0; i < MAXNUMBEROFFLAGS; i++)
                currentClient->flags[i] = 0;
            
            currentClient->startOverallTime = -1;
            currentClient->currentNumberOfSteps = 0;
            currentClient->lastNumberOfSteps = 0;
            currentClient->active = 0;
            
            currentClient = nextClient;
        }
    }
    
    else
    {
        while(i<argc)
        {
            if(atom_gettype(argv+i) == A_SYM)
                clientName = atom_getsym(argv+i);
            else if(atom_gettype(argv+i) == A_LONG)
            {
                sprintf(ID2oSym,"%ld", (long)atom_getlong(argv));
                clientName = gensym(ID2oSym);
            
            }
            else
                clientName = NULL;
                
            if(clientName)  //
            {
                currentClient = linklist_getindex(x->clientList, 0);
                while(currentClient != NULL)
                {
                    linklist_next(x->clientList, (void *)currentClient, (void *)&nextClient);
                    if(( clientName == currentClient->name) || clientName == gensym("all"))
                    {
                        currentClient->inWaitMode = 0;
                        currentClient->currentState = 0;
                        currentClient->lastState = -1;
                        currentClient->numberOfNo = 0;
                        currentClient->numberOfYes = 0;
                        currentClient->numberOfComply = 0;
                        currentClient->numberOfNoComply = 0;
                        currentClient->numberOfTimeOut = 0;
                        currentClient->currentPosition.x = -1;
                        currentClient->currentPosition.y = -1;
                        currentClient->lastPosition.x = -1;
                        currentClient->lastPosition.y = -1;
                        currentClient->currentPositionForDirection.x = -1;
                        currentClient->currentPositionForDirection.y = -1;
                        currentClient->lastPositionForDirection.x = -1;
                        currentClient->lastPositionForDirection.y = -1;
                        currentClient->walkingDirection = 0;
                        currentClient->lastOrientation = -1;
                        currentClient->currentOrientation = -1;
                        currentClient->lastYesOrNo = 0;
                        currentClient->rotation = 0;
                        currentClient->startOverallTime = -1;
                        currentClient->startTimeInCurrentState = 0;
                        currentClient->waitTimeInCurrentState = 0;
                        for(i=0; i < MAXNUMBEROFFLAGS; i++)
                            currentClient->flags[i] = 0;
                        
                        currentClient->startOverallTime = -1;
                        currentClient->currentNumberOfSteps = 0;
                        currentClient->lastNumberOfSteps = 0;
                        currentClient->active = 0;
                        post("Reset Client %s", clientName->s_name);
                    }
                    
                    currentClient = nextClient;
                }
            }
            
            i++;
        }
    }
}

//--------------------------------------------------------------------------------------------------------------------------------------//
//----------------------- fhnwAudiowalkState_stopWait ----------------------------------------------------------------------------------//
//----------------------- usefull for debugging ----------------------------------------------------------------------------------------//
//----------------------- ends a clients wait status -----------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------------------------//

void fhnwAudiowalkState_stopWait(t_fhnwAudiowalkState *x, t_symbol *message, short argc, t_atom *argv)
{
    fhnwAudiowalkState_client *currentClient, *nextClient;
    t_symbol *clientName = NULL;
    
    if(argc == 1)
    {
        if(atom_gettype(argv) != A_SYM)
            return;
        
        clientName = atom_getsym(argv);
        
        if(clientName)  //
        {
            currentClient = linklist_getindex(x->clientList, 0);
            while(currentClient != NULL)
            {
                linklist_next(x->clientList, (void *)currentClient, (void *)&nextClient);
                if(( clientName == currentClient->name) || clientName == gensym("all"))
                {
                    currentClient->startTimeInCurrentState = x->overallTime;
                    currentClient->waitTimeInCurrentState = 0;
                    currentClient->inWaitMode = 0;
                }
                
                linklist_next(x->clientList, (void *)currentClient, (void *)&nextClient);
                currentClient = nextClient;
            }
        }
    }
    else;
}

//--------------------------------------------------------------------------------------------------------------------------------------//
//----------------------- fhnwAudiowalkState_setState ----------------------------------------------------------------------------------//
//----------------------- usefull for debugging ----------------------------------------------------------------------------------------//
//----------------------- moves a client to a given state ------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------------------------//

void fhnwAudiowalkState_setState(t_fhnwAudiowalkState *x, t_symbol *message, short argc, t_atom *argv)
{
    fhnwAudiowalkState_client *currentClient, *nextClient;
    t_symbol *clientName = NULL;
    char intName[10];
    
    if(argc == 2)
    {
        if(atom_gettype(argv) == A_SYM)
            clientName = atom_getsym(argv);
        else if(atom_gettype(argv) == A_LONG)
        {
            sprintf(intName, "%d", (int)atom_getlong(argv));
            clientName = gensym(intName);
        }
        
        else
            return;
        
        if(atom_gettype(argv+1) != A_LONG)
            return;
        
        if(clientName)  //
        {
            currentClient = linklist_getindex(x->clientList, 0);
            while(currentClient != NULL)
            {
                linklist_next(x->clientList, (void *)currentClient, (void *)&nextClient);
                if(( clientName == currentClient->name) || clientName == gensym("all"))
                {
                    currentClient->currentState = atom_getlong(argv+1);
                    currentClient->startTimeInCurrentState = x->overallTime;
                    currentClient->waitTimeInCurrentState = 0;
                    currentClient->inWaitMode = 0;
                }
                currentClient = nextClient;
            }
        }
    }
    else;
}

void fhnwAudiowalkState_clear(t_fhnwAudiowalkState *x, t_symbol *message, short argc, t_atom *argv)
{
    fhnwAudiowalkState_reset(x, 0L,0, NULL);
    fhnwAudiowalkState_removeEvent(x, 0L, 0, NULL);
    fhnwAudiowalkState_removeClient(x, 0L, 0, NULL);
}

void fhnwAudiowalkState_start(t_fhnwAudiowalkState *x, t_symbol *message, short argc, t_atom *argv)
{
    fhnwAudiowalkState_client *currentClient, *nextClient;
    t_symbol *clientName = NULL;
    char ID2oSym[10];
    
    // no argument starts all clients and scheduler if it is not already running, usely the only thing we need
    
    if(!argc)
    {
        post("Started all clients");
        
        if(!x->onOff)
        {
            clock_fdelay(x->timer, 0);
            post("Start Scheduler");
            x->onOff = 1;
        }
        
        currentClient = linklist_getindex(x->clientList, 0);
        while(currentClient != NULL)
        {
            linklist_next(x->clientList, (void *)currentClient, (void *)&nextClient);
            currentClient->active = 1;
            if(currentClient->startOverallTime < 0)
                currentClient->startOverallTime = x->overallTime;
            
            currentClient = nextClient;
        }
    }
    
    else if(argc == 1)
    {
        if(atom_gettype(argv) == A_SYM)
        {
            // keyword scheduler starts only the scheduler
            
            if(atom_getsym(argv) == gensym("scheduler"))
            {
                if(!x->onOff)
                {
                    clock_fdelay(x->timer, 0);
                    post("Start Scheduler");
                    x->onOff = 1;
                }
            }
            
            // or if client name exists it starts the corresponding client
            
            else
            {
                clientName = atom_getsym(argv);
                
                if(clientName)  //
                {
                    currentClient = linklist_getindex(x->clientList, 0);
                    while(currentClient != NULL)
                    {
                        linklist_next(x->clientList, (void *)currentClient, (void *)&nextClient);
                        if(( clientName == currentClient->name) || clientName == gensym("all"))
                        {
                            post("Started client %s", clientName->s_name);
                            currentClient->active = 1;
                            if(currentClient->startOverallTime < 0)
                                currentClient->startOverallTime = x->overallTime;
                        }
                        
                        currentClient = nextClient;
                    }
                }
            }
        }
        
        // or if client name is an integer, it starts the corresponding client
        
        if(atom_gettype(argv) == A_LONG)
        {
            sprintf(ID2oSym,"%ld", (long)atom_getlong(argv));
            clientName = gensym(ID2oSym);
            
            if(clientName)  //
            {
                currentClient = linklist_getindex(x->clientList, 0);
                while(currentClient != NULL)
                {
                    linklist_next(x->clientList, (void *)currentClient, (void *)&nextClient);
                    if(( clientName == currentClient->name) || clientName == gensym("all"))
                    {
                        post("Started client %s", clientName->s_name);
                        currentClient->active = 1;
                        if(currentClient->startOverallTime < 0)
                            currentClient->startOverallTime = x->overallTime;
                    }
                    
                    currentClient = nextClient;
                }
            }
        }
    }
    
    // not in use
    else
    {
        //to implement: list of atoms with keywords and/or client names as integers or symbols
    }
}

void fhnwAudiowalkState_stop(t_fhnwAudiowalkState *x, t_symbol *message, short argc, t_atom *argv)
{
    fhnwAudiowalkState_client *currentClient, *nextClient;
    t_symbol *clientName = NULL;
    char intName[10];
    int i = 0;
    
    if(!argc)
    {
        currentClient = linklist_getindex(x->clientList, 0);
        while(currentClient != NULL)
        {
            linklist_next(x->clientList, (void *)currentClient, (void *)&nextClient);
            post("Stopped all clients");
            currentClient->active = 0;
            currentClient = nextClient;
        }
        
        clock_unset(x->timer);
        post("Stopped Scheduler");
        x->onOff = 0;
    }
    
    else
    {
        while(i<argc)
        {
            if(atom_gettype(argv) == A_SYM)
                clientName = atom_getsym(argv);
            else if(atom_gettype(argv) == A_LONG)
            {
                sprintf(intName, "%d", (int)atom_getlong(argv));
                clientName = gensym(intName);
            }
            else
                clientName = NULL;
                
            if(clientName)  //
            {
                currentClient = linklist_getindex(x->clientList, 0);
                while(currentClient != NULL)
                {
                    linklist_next(x->clientList, (void *)currentClient, (void *)&nextClient);
                    if(( clientName == currentClient->name) || clientName == gensym("all"))
                    {
                        post("Stopped client %s", clientName->s_name);
                        currentClient->active = 0;
                    }
                    
                    currentClient = nextClient;
                }
            }
            i++;
        }
    }
}

//--------------------------------------------------------------------------------------------------------------------------------------//
//----------------------- fhnwAudiowalkState_startTimer -------------------------------------------------------------------------------------------//
//------------------------not in use yet -----------------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------------------------//

void fhnwAudiowalkState_startTimer(t_fhnwAudiowalkState *x, t_symbol *message, short argc, t_atom *argv)
{
    fhnwAudiowalkState_client *currentClient, *nextClient;
    long ID;
    if(argc >= 1)
    {
        ID = atom_getlong(argv);
        
        currentClient = linklist_getindex(x->clientList, 0);
        while(currentClient != NULL)
        {
            linklist_next(x->clientList, (void *)currentClient, (void *)&nextClient);
            if(ID == currentClient->ID)
            {
                currentClient->startOverallTime = x->overallTime;
                break;
            }
            currentClient = nextClient;
        }
    }
}

//--------------------------------------------------------------------------------------------------------------------------------------//
//----------------------- fhnwAudiowalkState_createClient ------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------------------------//

void fhnwAudiowalkState_newClient(t_fhnwAudiowalkState *x, t_symbol *message, short argc, t_atom *argv)
{
    int i = 0;
    fhnwAudiowalkState_client *p = (fhnwAudiowalkState_client *)sysmem_newptr(sizeof(fhnwAudiowalkState_client));
    char intName[10];
    p->ID = x->clientIDCounter++;
    p->active = 0;
    p->group = 1;
    p->currentState = 0;
    p->lastState = -1;
    p->numberOfNo = 0;
    p->numberOfYes = 0;
    p->numberOfComply = 0;
    p->numberOfNoComply = 0;
    p->numberOfTimeOut = 0;
    p->currentPosition.x = -1;
    p->currentPosition.y = -1;
    p->lastPosition.x = -1;
    p->lastPosition.y = -1;
    p->currentPositionForDirection.x = -1;
    p->currentPositionForDirection.y = -1;
    p->lastPositionForDirection.x = -1;
    p->lastPositionForDirection.y = -1;
    p->walkingDirection = 0;
    p->lastOrientation = 0;
    p->lastYesOrNo = 0;
    p->rotation = 0;
    p->startOverallTime = -1;
    p->startTimeInCurrentState = 0;
    p->waitTimeInCurrentState = 0;
    p->currentNumberOfSteps = 0;
    p->lastNumberOfSteps = 0;
    
    for(i=0; i < MAXNUMBEROFFLAGS; i++)
        p->flags[i] = 0;
    
    p->name = gensym("noName");
    
    if(argc == 1)
    {
        if(atom_gettype(argv) == A_SYM)
            p->name = atom_getsym(argv);
        if(atom_gettype(argv) == A_LONG)
        {
            sprintf(intName, "%d", (int)atom_getlong(argv));
            p->name = gensym(intName);
            p->ID = atom_getlong(argv);
        }
    }
    
    x->numberOfClients++;
    linklist_append(x->clientList, (void *)p);
}

void fhnwAudiowalkState_removeClient(t_fhnwAudiowalkState *x, t_symbol *message, short argc, t_atom *argv)
{
    fhnwAudiowalkState_client *p, *nextClient;
    t_symbol *ID;
    if(argc >= 1)
    {
        if(atom_gettype(argv) == A_SYM)
        {
            ID = atom_getsym(argv);
            
            p = linklist_getindex(x->clientList, 0);
            while(p != NULL)
            {
                linklist_next(x->clientList, (void *)p, (void *)&nextClient);
                if(ID == p->name)
                {
                    linklist_deleteobject(x->clientList, p);
                    x->numberOfClients--;
                    break;
                }
                else
                    p = nextClient;
            }
        }
        else // deletes all
        {
            p = linklist_getindex(x->clientList, 0);
            while(p != NULL)
            {
                linklist_next(x->clientList, (void *)p, (void *)&nextClient);
                linklist_deleteobject(x->clientList, p);
                p = nextClient;
            }
            x->numberOfClients = 0;
            x->clientIDCounter = 1;
        }
    }
    
    else // deletes all
    {
        p = linklist_getindex(x->clientList, 0);
        while(p != NULL)
        {
            linklist_next(x->clientList, (void *)p, (void *)&nextClient);
            linklist_deleteobject(x->clientList, p);
            p = nextClient;
        }
        x->numberOfClients = 0;
        x->clientIDCounter = 1;
    }
}

//--------------------------------------------------------------------------------------------------------------------------------------//
//----------------------- fhnwAudiowalkState_nextState ---------------------------------------------------------------------------------//
//----------------------- adds a transition either for a specific client or for every client  ------------------------------------------//
//------------------------from a current state to the next state if the specified conditions -------------------------------------------//
//------------------------are fulfilled ------------------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------------------------//

void fhnwAudiowalkState_nextState(t_fhnwAudiowalkState *x, t_symbol *message, short argc, t_atom *argv)
{
    t_symbol *conditionType;
    fhnwAudiowalkState_condition *c[20];
    fhnwAudiowalkState_message *m[20];
	int i = 0;
    int j = 0;
    int conditionCount = 0;
    int messageCount = 0;
    char ID2oSym[10];
    
    fhnwAudiowalkState_event *e = (fhnwAudiowalkState_event *)sysmem_newptr(sizeof(fhnwAudiowalkState_event));
    e->conditions = (t_linklist *)linklist_new();
    e->messages = (t_linklist *)linklist_new();
    e->flag2set = -1;
    e->flag2unset = -1;
    e->wait = 0;
    
    linklist_flags(e->conditions, OBJ_FLAG_MEMORY);
    linklist_flags(e->messages, OBJ_FLAG_MEMORY);
    
    if(argc >= 3)
    {
        if(atom_gettype(argv) == A_LONG)
        {
            e->nextState = atom_getlong(argv+i);
            i++;
        }
        else
            goto CREATEEVENTEND;
    }
    
    while(i < argc)
    {
        if(atom_gettype(argv+i) == A_SYM)
        {
            conditionType = atom_getsym(argv+i);
            
            if(conditionType == gensym("ID"))
            {
                i++;
				if(i >= argc) //missing arg for ID
					goto CREATEEVENTEND;
                
                if(atom_gettype(argv+i) == A_SYM)
                {
                    c[conditionCount] = (fhnwAudiowalkState_condition *)sysmem_newptr(sizeof(fhnwAudiowalkState_condition));
                    c[conditionCount]->type = EVENTCONDITION_CLIENTID;
                    
                    atom_setsym(&c[conditionCount]->values[0], atom_getsym(argv+i));
                    linklist_append(e->conditions,  c[conditionCount]);
                    
                    i++;
                    conditionCount++;
                }
                
                else if(atom_gettype(argv+i) == A_LONG)
                {
                    c[conditionCount] = (fhnwAudiowalkState_condition *)sysmem_newptr(sizeof(fhnwAudiowalkState_condition));
                    c[conditionCount]->type = EVENTCONDITION_CLIENTID;
                    sprintf(ID2oSym,"%ld", (long)atom_getlong(argv+i));
                    atom_setsym(&c[conditionCount]->values[0], gensym(ID2oSym));
                    linklist_append(e->conditions,  c[conditionCount]);
                    
                    i++;
                    conditionCount++;
                }
                else
                    goto CREATEEVENTEND;
            }
            //--------------------------------------------------------------------------------------------------------------------------------------//
            //-------------------------------- Allowing "CURRENTSTATE" makes the scripts more readable ---------------------------------------------//
            //--------------------------------------------------------------------------------------------------------------------------------------//
            
            else if(conditionType == gensym("state") || conditionType == gensym("currentState") || conditionType == gensym("CURRENTSTATE") )
            {
                i++;
				if(i >= argc)
					goto CREATEEVENTEND;
                
                if(atom_gettype(argv+i) == A_LONG)
                {
                    c[conditionCount] = (fhnwAudiowalkState_condition *)sysmem_newptr(sizeof(fhnwAudiowalkState_condition));
                    c[conditionCount]->type = EVENTCONDITION_STATE;
                    
                    atom_setlong(&c[conditionCount]->values[j], atom_getlong(argv+i));
                    linklist_append(e->conditions,  c[conditionCount]);
                    
                    i++;
                    conditionCount++;
                }
                else
                    goto CREATEEVENTEND;
            }
            
            else if(conditionType == gensym("yes"))
            {
                i++;
                c[conditionCount] = (fhnwAudiowalkState_condition *)sysmem_newptr(sizeof(fhnwAudiowalkState_condition));
                c[conditionCount]->type = EVENTCONDITION_YES;
                linklist_append(e->conditions,  c[conditionCount]);
                conditionCount++;
            }
            
            else if(conditionType == gensym("no"))
            {
                i++;
                c[conditionCount] = (fhnwAudiowalkState_condition *)sysmem_newptr(sizeof(fhnwAudiowalkState_condition));
                c[conditionCount]->type = EVENTCONDITION_NO;
                linklist_append(e->conditions,  c[conditionCount]);
                conditionCount++;
            }
            
            else if(conditionType == gensym("getUp"))
            {
                i++;
                c[conditionCount] = (fhnwAudiowalkState_condition *)sysmem_newptr(sizeof(fhnwAudiowalkState_condition));
                c[conditionCount]->type = EVENTCONDITION_GETUP;
                linklist_append(e->conditions,  c[conditionCount]);
                conditionCount++;
            }
            
            else if(conditionType == gensym("sitDown"))
            {
                i++;
                c[conditionCount] = (fhnwAudiowalkState_condition *)sysmem_newptr(sizeof(fhnwAudiowalkState_condition));
                c[conditionCount]->type = EVENTCONDITION_SITDOWN;
                linklist_append(e->conditions,  c[conditionCount]);
                conditionCount++;
            }
            
            else if(conditionType == gensym("comply"))
            {
                i++;
                c[conditionCount] = (fhnwAudiowalkState_condition *)sysmem_newptr(sizeof(fhnwAudiowalkState_condition));
                c[conditionCount]->type = EVENTCONDITION_COMPLY;
                linklist_append(e->conditions,  c[conditionCount]);
                conditionCount++;
            }
            
            else if(conditionType == gensym("noComply"))
            {
                i++;
                c[conditionCount] = (fhnwAudiowalkState_condition *)sysmem_newptr(sizeof(fhnwAudiowalkState_condition));
                c[conditionCount]->type = EVENTCONDITION_NOCOMPLY;
                linklist_append(e->conditions,  c[conditionCount]);
                conditionCount++;
            }
            
            else if(conditionType == gensym("position"))
            {
                i++;
                
				if(i+3 >= argc)
				{
					error("fhnw.audiowalk.state: Wrong number of Arguments");
					goto CREATEEVENTEND;
				}
                
                for(j=0; j<4; j++)
                {
                    if(atom_gettype(argv+i+j) != A_FLOAT)
                        goto CREATEEVENTEND;
                }
                
                c[conditionCount] = (fhnwAudiowalkState_condition *)sysmem_newptr(sizeof(fhnwAudiowalkState_condition));
                c[conditionCount]->type = EVENTCONDITION_POSITION;
                for(j=0; j<4; j++)
                    atom_setfloat(&c[conditionCount]->values[j], atom_getfloat(argv+i+j));
                
                linklist_append(e->conditions,  c[conditionCount]);
                
                i+=4;
                conditionCount++;
            }
            
            else if(conditionType == gensym("!position"))
            {
                i++;
				if(i+3 >= argc)
				{
					error("fhnw.audiowalk.state: Wrong number of Arguments");
					goto CREATEEVENTEND;
				}
                
                for(j=0; j<4; j++)
                {
                    if(atom_gettype(argv+i+j) != A_FLOAT)
                        goto CREATEEVENTEND;
                }
                
                c[conditionCount] = (fhnwAudiowalkState_condition *)sysmem_newptr(sizeof(fhnwAudiowalkState_condition));
                c[conditionCount]->type = EVENTCONDITION_NOTPOSITION;
                for(j=0; j<4; j++)
                    atom_setfloat(&c[conditionCount]->values[j], atom_getfloat(argv+i+j));
                
                linklist_append(e->conditions,  c[conditionCount]);
                
                i+=4;
                conditionCount++;
            }
            
            //--------------------------------------------------------------------------------------------------------------------------------------//
            //-------------------------------- allows to check whether 2 clients are standing close to each other ----------------------------------//
            
            else if(conditionType == gensym("closeBy"))
            {
                i++;
                
				if(i+1 >= argc)
					goto CREATEEVENTEND;
                
                if(atom_gettype(argv+i) == A_SYM && atom_gettype(argv+i+1) == A_FLOAT)
                {
                    c[conditionCount] = (fhnwAudiowalkState_condition *)sysmem_newptr(sizeof(fhnwAudiowalkState_condition));
                    c[conditionCount]->type = EVENTCONDITION_CLOSEBY;
                    
                    atom_setsym(&c[conditionCount]->values[0], atom_getsym(argv+i));
                    atom_setfloat(&c[conditionCount]->values[1], atom_getfloat(argv+i+1));
                    linklist_append(e->conditions,  c[conditionCount]);
                    
                    i++;i++;
                    conditionCount++;
                }
                else
                    goto CREATEEVENTEND;
            }
            
            //--------------------------------------------------------------------------------------------------------------------------------------//
            //-------------------------------- allows to check whether position has changed more than [float] --------------------------------------//
            
            else if(conditionType == gensym("positionChanged"))
            {
                i++;
                
				if(i >= argc)
				{
					error("fhnw.audiowalk.state: Wrong number of Arguments");
					goto CREATEEVENTEND;
				}
                
                if(atom_gettype(argv+i) == A_FLOAT)
                {
                    c[conditionCount] = (fhnwAudiowalkState_condition *)sysmem_newptr(sizeof(fhnwAudiowalkState_condition));
                    c[conditionCount]->type = EVENTCONDITION_POSITIONCHANGED;
                    
                    atom_setfloat(&c[conditionCount]->values[0], atom_getfloat(argv+i));
                    linklist_append(e->conditions,  c[conditionCount]);
                    
                    i++;
                    conditionCount++;
                }
                else
                    goto CREATEEVENTEND;
            }
            
            //--------------------------------------------------------------------------------------------------------------------------------------//
            //-------------------------------- allows to check whether position has changed less than [float] --------------------------------------//
            
            else if(conditionType == gensym("!positionChanged"))
            {
                i++;
				if(i >= argc)
				{
					error("fhnw.audiowalk.state: Wrong number of Arguments");
					goto CREATEEVENTEND;
				}
                
                if(atom_gettype(argv+i) == A_FLOAT)
                {
                    c[conditionCount] = (fhnwAudiowalkState_condition *)sysmem_newptr(sizeof(fhnwAudiowalkState_condition));
                    c[conditionCount]->type = EVENTCONDITION_NOTPOSITIONCHANGED;
                    
                    atom_setfloat(&c[conditionCount]->values[0], atom_getfloat(argv+i));
                    linklist_append(e->conditions,  c[conditionCount]);
                    
                    i++;
                    conditionCount++;
                }
                else
                    goto CREATEEVENTEND;
            }
            
            //--------------------------------------------------------------------------------------------------------------------------------------//
            //-------------------------------- allows to check the absolute orientation in degrees[0- 360], quantized with 15 ----------------------//
            
            else if(conditionType == gensym("orientation"))
            {
                i++;
                
				if(i >= argc)
				{
					error("fhnw.audiowalk.state: Wrong number of Arguments");
					goto CREATEEVENTEND;
				}
                
                if(atom_gettype(argv+i) == A_LONG)
                {
                    c[conditionCount] = (fhnwAudiowalkState_condition *)sysmem_newptr(sizeof(fhnwAudiowalkState_condition));
                    c[conditionCount]->type = EVENTCONDITION_ORIENTATION;
                    
                    atom_setlong(&c[conditionCount]->values[0], atom_getlong(argv+i));
                    atom_setlong(&c[conditionCount]->values[1], atom_getlong(argv+i)+15);
                    i++;
                    if(i < argc)
                    {
                        if(atom_gettype(argv+i) == A_LONG)
                        {
                            atom_setlong(&c[conditionCount]->values[1], atom_getlong(argv+i));
                            i++;
                        }
                    }
                    
                    linklist_append(e->conditions,  c[conditionCount]);
                    conditionCount++;
                }
            }
            
            else if(conditionType == gensym("walkingDirection"))
            {
                i++;
				if(i >= argc)
				{
					error("fhnw.audiowalk.state: Wrong number of Arguments");
					goto CREATEEVENTEND;
				}
                
                if(atom_gettype(argv+i) == A_LONG)
                {
                    c[conditionCount] = (fhnwAudiowalkState_condition *)sysmem_newptr(sizeof(fhnwAudiowalkState_condition));
                    c[conditionCount]->type = EVENTCONDITION_WALKINGDIRECTION;
                    
                    atom_setlong(&c[conditionCount]->values[0], atom_getlong(argv+i));
                    linklist_append(e->conditions,  c[conditionCount]);
                    
                    i++;
                    conditionCount++;
                }
            }
            
            //--------------------------------------------------------------------------------------------------------------------------------------//
            //-------------------------------- rotation either clockwise [1] or counterclockwise [-1] ----------------------------------------------//
            
            else if(conditionType == gensym("rotation"))
            {
                i++;
				if(i >= argc)
				{
					error("fhnw.audiowalk.state: Wrong number of Arguments");
					goto CREATEEVENTEND;
				}
                
                if(atom_gettype(argv+i) == A_LONG)
                {
                    c[conditionCount] = (fhnwAudiowalkState_condition *)sysmem_newptr(sizeof(fhnwAudiowalkState_condition));
                    c[conditionCount]->type = EVENTCONDITION_ROTATION;
                    
                    atom_setlong(&c[conditionCount]->values[0], atom_getlong(argv+i));
                    linklist_append(e->conditions,  c[conditionCount]);
                    
                    i++;
                    conditionCount++;
                }
            }
            
            else if(conditionType == gensym("numberOfSteps"))
            {
                i++;
				if(i >= argc)
				{
					error("fhnw.audiowalk.state: Wrong number of Arguments");
					goto CREATEEVENTEND;
				}
                
                if(atom_gettype(argv+i) == A_LONG)
                {
                    c[conditionCount] = (fhnwAudiowalkState_condition *)sysmem_newptr(sizeof(fhnwAudiowalkState_condition));
                    c[conditionCount]->type = EVENTCONDITION_NUMBEROFSTEPSINCURRENTSTATE;
                    
                    atom_setlong(&c[conditionCount]->values[0], atom_getlong(argv+i));
                    linklist_append(e->conditions,  c[conditionCount]);
                    
                    i++;
                    conditionCount++;
                }
            }
            
            else if(conditionType == gensym("numberOfStepsOverall"))
            {
                i++;
				if(i >= argc)
				{
					error("fhnw.audiowalk.state: Wrong number of Arguments");
					goto CREATEEVENTEND;
				}
                
                if(atom_gettype(argv+i) == A_LONG)
                {
                    c[conditionCount] = (fhnwAudiowalkState_condition *)sysmem_newptr(sizeof(fhnwAudiowalkState_condition));
                    c[conditionCount]->type = EVENTCONDITION_NUMBEROFSTEPS;
                    
                    atom_setlong(&c[conditionCount]->values[0], atom_getlong(argv+i));
                    linklist_append(e->conditions,  c[conditionCount]);
                    
                    i++;
                    conditionCount++;
                }
            }
            
            //--------------------------------------------------------------------------------------------------------------------------------------//
            //-------------------------------- allows to check whether orientation has changed more than [long] --------------------------------------//
            
            else if(conditionType == gensym("orientationChanged"))
            {
                i++;
				if(i >= argc)
				{
					error("fhnw.audiowalk.state: Wrong number of Arguments");
					goto CREATEEVENTEND;
				}
                
                if(atom_gettype(argv+i) == A_LONG)
                {
                    c[conditionCount] = (fhnwAudiowalkState_condition *)sysmem_newptr(sizeof(fhnwAudiowalkState_condition));
                    c[conditionCount]->type = EVENTCONDITION_ORIENTATIONCHANGED;
                    
                    atom_setlong(&c[conditionCount]->values[0], atom_getlong(argv+i));
                    linklist_append(e->conditions,  c[conditionCount]);
                    
                    i++;
                    conditionCount++;
                }
            }
            
            //--------------------------------------------------------------------------------------------------------------------------------------//
            //-------------------------------- allows to check whether orientation has changed less than [long] --------------------------------------//
            
            else if(conditionType == gensym("!orientationChanged"))
            {
                i++;
				if(i >= argc)
				{
					error("fhnw.audiowalk.state: Wrong number of Arguments");
					goto CREATEEVENTEND;
				}
                
                if(atom_gettype(argv+i) == A_LONG)
                {
                    c[conditionCount] = (fhnwAudiowalkState_condition *)sysmem_newptr(sizeof(fhnwAudiowalkState_condition));
                    c[conditionCount]->type = EVENTCONDITION_NOTORIENTATIONCHANGED;
                    
                    atom_setlong(&c[conditionCount]->values[0], atom_getlong(argv+i));
                    linklist_append(e->conditions,  c[conditionCount]);
                    
                    i++;
                    conditionCount++;
                }
            }
            
            else if(conditionType == gensym("maxNumberNo") || conditionType == gensym("numberOfNo"))
            {
                i++;
				if(i >= argc)
				{
					error("fhnw.audiowalk.state: Wrong number of Arguments");
					goto CREATEEVENTEND;
				}
                
                if(atom_gettype(argv+i) == A_LONG)
                {
                    c[conditionCount] = (fhnwAudiowalkState_condition *)sysmem_newptr(sizeof(fhnwAudiowalkState_condition));
                    c[conditionCount]->type = EVENTCONDITION_NUMBEROFNO;
                    
                    atom_setlong(&c[conditionCount]->values[0], atom_getlong(argv+i));
                    linklist_append(e->conditions,  c[conditionCount]);
                    
                    i++;
                    conditionCount++;
                }
                else
                    goto CREATEEVENTEND;
            }
            
            else if(conditionType == gensym("maxNumberYes") || conditionType == gensym("numberOfYes"))
            {
                i++;
				if(i >= argc)
				{
					error("fhnw.audiowalk.state: Wrong number of Arguments");
					goto CREATEEVENTEND;
				}
                
                if(atom_gettype(argv+i) == A_LONG)
                {
                    c[conditionCount] = (fhnwAudiowalkState_condition *)sysmem_newptr(sizeof(fhnwAudiowalkState_condition));
                    c[conditionCount]->type = EVENTCONDITION_NUMBEROFYES;
                    
                    atom_setlong(&c[conditionCount]->values[0], atom_getlong(argv+i));
                    linklist_append(e->conditions,  c[conditionCount]);
                    
                    i++;
                    conditionCount++;
                }
                else
                    goto CREATEEVENTEND;
            }
            
            //--------------------------------------------------------------------------------------------------------------------------------------//
            //------------------------------------------- timeOuts are counted, maxTime not---------------------------------------------------------//
            
            else if(conditionType == gensym("maxNumberTimeOut") || conditionType == gensym("numberOfTimeOut"))
            {
                i++;
				if(i >= argc)
				{
					error("fhnw.audiowalk.state: Wrong number of Arguments");
					goto CREATEEVENTEND;
				}
                
                if(atom_gettype(argv+i) == A_LONG)
                {
                    c[conditionCount] = (fhnwAudiowalkState_condition *)sysmem_newptr(sizeof(fhnwAudiowalkState_condition));
                    c[conditionCount]->type = EVENTCONDITION_NUMBEROFTIMEOUT;
                    
                    atom_setlong(&c[conditionCount]->values[0], atom_getlong(argv+i));
                    linklist_append(e->conditions,  c[conditionCount]);
                    
                    i++;
                    conditionCount++;
                }
                else
                    goto CREATEEVENTEND;
            }
            
            else if(conditionType == gensym("maxTime"))
            {
                i++;
				if(i >= argc)
				{
					error("fhnw.audiowalk.state: Wrong number of Arguments");
					goto CREATEEVENTEND;
				}
                
                if(atom_gettype(argv+i) == A_FLOAT)
                {
                    c[conditionCount] = (fhnwAudiowalkState_condition *)sysmem_newptr(sizeof(fhnwAudiowalkState_condition));
                    c[conditionCount]->type = EVENTCONDITION_ELAPSEDTIMEINCURRENTSTATE;
                    
                    atom_setfloat(&c[conditionCount]->values[0], atom_getfloat(argv+i));
                    linklist_append(e->conditions,  c[conditionCount]);
                    
                    i++;
                    conditionCount++;
                }
                else
                    goto CREATEEVENTEND;
            }
            
            else if(conditionType == gensym("timeRange"))
            {
                i++;
				if(i+1 >= argc)
				{
					error("fhnw.audiowalk.state: Wrong number of Arguments");
					goto CREATEEVENTEND;
				}
                
                if(atom_gettype(argv+i) == A_FLOAT)
                {
                    if(atom_gettype(argv+i+1) != A_FLOAT)
                        goto CREATEEVENTEND;
                    
                    c[conditionCount] = (fhnwAudiowalkState_condition *)sysmem_newptr(sizeof(fhnwAudiowalkState_condition));
                    c[conditionCount]->type = EVENTCONDITION_TIMERANGEINCURRENTSTATE;
                    
                    atom_setfloat(&c[conditionCount]->values[0], atom_getfloat(argv+i));
                    atom_setfloat(&c[conditionCount]->values[1], atom_getfloat(argv+i+1));
                    linklist_append(e->conditions,  c[conditionCount]);
                    
                    i+=2;
                    conditionCount++;
                }
                else
                    goto CREATEEVENTEND;
            }
            
            else if(conditionType == gensym("timeOut"))
            {
                i++;
				if(i >= argc)
				{
					error("fhnw.audiowalk.state: Wrong number of Arguments");
					goto CREATEEVENTEND;
				}
                
                if(atom_gettype(argv+i) == A_FLOAT)
                {
                    c[conditionCount] = (fhnwAudiowalkState_condition *)sysmem_newptr(sizeof(fhnwAudiowalkState_condition));
                    c[conditionCount]->type = EVENTCONDITION_ELAPSEDTIMEINCURRENTSTATE_TIMEOUT;
                    
                    atom_setfloat(&c[conditionCount]->values[0], atom_getfloat(argv+i));
                    linklist_append(e->conditions,  c[conditionCount]);
                    
                    i++;
                    conditionCount++;
                }
                else
                    goto CREATEEVENTEND;
            }
            
            //--------------------------------------------------------------------------------------------------------------------------------------//
            //----------------------------puts position in wait mode for x ms after before moving to the next state---------------------------------//
            
            else if(conditionType == gensym("wait"))
            {
                i++;
                if(atom_gettype(argv+i) == A_FLOAT)
                {
                    e->wait = atom_getfloat(argv+i);
                    i++;
                }
                else
                    goto CREATEEVENTEND;
            }
            
            else if(conditionType == gensym("maxOverallTime"))
            {
                i++;
				if(i >= argc)
				{
					error("fhnw.audiowalk.state: Wrong number of Arguments");
					goto CREATEEVENTEND;
				}
                
                if(atom_gettype(argv+i) == A_FLOAT)
                {
                    c[conditionCount] = (fhnwAudiowalkState_condition *)sysmem_newptr(sizeof(fhnwAudiowalkState_condition));
                    c[conditionCount]->type = EVENTCONDITION_ELAPSEDOVERALLTIME;
                    
                    atom_setfloat(&c[conditionCount]->values[0], atom_getfloat(argv+i));
                    linklist_append(e->conditions,  c[conditionCount]);
                    
                    i++;
                    conditionCount++;
                }
                else
                    goto CREATEEVENTEND;
            }
            
            //--------------------------------------------------------------------------------------------------------------------------------------//
            //---------------------------- messages to be send after fulfilling conditions and moving to next state --------------------------------//
            
            else if(conditionType == gensym("message"))
            {
                i++;
				if(i+1 >= argc)
				{
					error("fhnw.audiowalk.state: Wrong number of Arguments");
					goto CREATEEVENTEND;
				}
                
                for(j=0; j<2; j++)
                {
                    if(atom_gettype(argv+i+j) != A_SYM)
                        goto CREATEEVENTEND;
                }
                
                m[messageCount] = (fhnwAudiowalkState_message *)sysmem_newptr(sizeof(fhnwAudiowalkState_message));
                m[messageCount]->target = atom_getsym(argv+i);
                i++;
                
                m[messageCount]->message = atom_getsym(argv+i);
                i++;
                
                linklist_append(e->messages,  m[messageCount]);
                messageCount++;
                
            }
            
            else if(conditionType == gensym("flag"))
            {
                i++;
				if(i >= argc)
				{
					error("fhnw.audiowalk.state: Wrong number of Arguments");
					goto CREATEEVENTEND;
				}
				
                if(atom_gettype(argv+i) == A_LONG)
                {
                    c[conditionCount] = (fhnwAudiowalkState_condition *)sysmem_newptr(sizeof(fhnwAudiowalkState_condition));
                    c[conditionCount]->type = EVENTCONDITION_FLAGS;
                    
                    atom_setlong(&c[conditionCount]->values[0], atom_getlong(argv+i));
                    linklist_append(e->conditions,  c[conditionCount]);
                    
                    i++;
                    conditionCount++;
                }
                else
                    goto CREATEEVENTEND;
            }
            
            else if(conditionType == gensym("noFlag") || conditionType == gensym("!flag"))
            {
                i++;
				if(i >= argc)
				{
					error("fhnw.audiowalk.state: Wrong number of Arguments");
					goto CREATEEVENTEND;
				}
                if(atom_gettype(argv+i) == A_LONG)
                {
                    c[conditionCount] = (fhnwAudiowalkState_condition *)sysmem_newptr(sizeof(fhnwAudiowalkState_condition));
                    c[conditionCount]->type = EVENTCONDITION_NOFLAGS;
                    
                    atom_setlong(&c[conditionCount]->values[0], atom_getlong(argv+i));
                    linklist_append(e->conditions,  c[conditionCount]);
                    
                    i++;
                    conditionCount++;
                }
                else
                    goto CREATEEVENTEND;
            }
            
            else if(conditionType == gensym("setFlag"))
            {
                i++;
				if(i >= argc)
				{
					error("fhnw.audiowalk.state: Wrong number of Arguments");
					goto CREATEEVENTEND;
				}
                
                if(atom_gettype(argv+i) == A_LONG)
                {
                    e->flag2set = atom_getlong(argv+i);
                    i++;
                    
                }
                else
                    goto CREATEEVENTEND;
            }
            else if(conditionType == gensym("unsetFlag"))
            {
                i++;
				if(i >= argc)
				{
					error("fhnw.audiowalk.state: Wrong number of Arguments");
					goto CREATEEVENTEND;
				}
                
                if(atom_gettype(argv+i) == A_LONG)
                {
                    e->flag2unset = atom_getlong(argv+i);
                    i++;
                    
                }
                else
                    goto CREATEEVENTEND;
            }
            else
            {
                i++;
                c[conditionCount] = (fhnwAudiowalkState_condition *)sysmem_newptr(sizeof(fhnwAudiowalkState_condition));
                c[conditionCount]->type = EVENTCONDITION_ANYMESSAGE;
                atom_setsym(c[conditionCount]->values, conditionType);
                linklist_append(e->conditions,  c[conditionCount]);
                conditionCount++;
            }
        }
    }
    e->ID = x->eventIDCounter++;
    
    linklist_append(x->eventList, e);
    
    x->numberOfEvents++;
    return;
    
CREATEEVENTEND: // cleaning up if something went wrong
    linklist_clear(e->conditions);
    linklist_clear(e->messages);
    sysmem_freeptr(e);
}

void fhnwAudiowalkState_removeEvent(t_fhnwAudiowalkState *x, t_symbol *message, short argc, t_atom *argv)
{
    fhnwAudiowalkState_event *e, *nextEvent;
    long ID;
    if(argc >= 1)
    {
        
        // not in use, since there is no access to event ID's yet
        if(atom_gettype(argv) == A_LONG)
        {
            ID = atom_getlong(argv);
            e = linklist_getindex(x->eventList, 0);
            
            while(e != NULL)
            {
                if(e->ID == ID)
                {
                    linklist_clear(e->conditions);
                    linklist_clear(e->messages);
                    linklist_deleteobject(x->eventList, e);
                    x->numberOfEvents--;
                    break;
                }
                else
                {
                    linklist_next(x->eventList, (void *)e, (void *)&nextEvent);
                    e = nextEvent;
                }
            }
        }
        else // remove All
        {
            e = linklist_getindex(x->eventList, 0);
            
            while(e != NULL)
            {
                linklist_next(x->eventList, (void *)e, (void *)&nextEvent);
                linklist_clear(e->conditions);
                linklist_clear(e->messages);
                linklist_deleteobject(x->eventList, e);
                e = nextEvent;
            }
            
            x->numberOfEvents = 0;
            x->eventIDCounter = 1;
        }
    }
    else // remove All
    {
        e = linklist_getindex(x->eventList, 0);
        
        while(e != NULL)
        {
            linklist_next(x->eventList, (void *)e, (void *)&nextEvent);
            linklist_clear(e->conditions);
            linklist_clear(e->messages);
            linklist_deleteobject(x->eventList, e);
            e = nextEvent;
        }
        
        x->numberOfEvents = 0;
        x->eventIDCounter = 1;
    }
}

void fhnwAudiowalkState_processStateAndMessagesAndFlags(t_fhnwAudiowalkState *x,  t_symbol *message, fhnwAudiowalkState_client *p, fhnwAudiowalkState_event *e)
{
    fhnwAudiowalkState_message *currentMessage, *nextMessage;
    t_atom out[40];
    
    if(p != NULL)
    {
        if(e->nextState >= 0)
        {
            p->lastState = p->currentState;
            p->currentState = e->nextState;
            p->startTimeInCurrentState = x->overallTime;
            
        }
        if(e->flag2set >= 0)
        {
            p->flags[e->flag2set] = 1;
        }
        if(e->flag2unset >= 0)
        {
            p->flags[e->flag2unset] = 0;
        }
        if(e->wait)
        {
            p->waitTimeInCurrentState = e->wait;
            p->inWaitMode = 1;
        }
    }
    
    currentMessage = linklist_getindex(e->messages, 0);
    while(currentMessage != NULL)
    {
        if(currentMessage->target == gensym("sender"))
        {
            atom_setsym(out, p->name);
            atom_setsym(out+1, currentMessage->message);
        }
        
        else
        {
            atom_setsym(out, currentMessage->target);
            atom_setsym(out+1, currentMessage->message);
        }
        
        defer(x->outlet, (method)outlet_list, 0L, 2, out);
        
        linklist_next(e->messages, (void *)currentMessage, (void *)&nextMessage);
        currentMessage = nextMessage;
    }
}

int fhnwAudiowalkState_checkConditions(t_fhnwAudiowalkState *x,  t_symbol *message, fhnwAudiowalkState_client *p, fhnwAudiowalkState_event *e, long flags)
{
    int fulfilledConditions = 1;
    t_rect position2check;
    long orientation2check;
    long orientation2checkRange;
	int maxTimeCount = 0; int no = 0; int yes = 0; int comply = 0; int position = 0; int orientation = 0;
    float distance, distance2check;
    fhnwAudiowalkState_condition *currentCondition, *nextCondition;
    currentCondition = linklist_getindex(e->conditions, 0);
    
    while(currentCondition != NULL)
    {
        if(  currentCondition->type == (EVENTCONDITION_CLIENTID & flags)  )
        {
            if(p->name != atom_getsym(&currentCondition->values[0]))
            {
                fulfilledConditions = 0;
                return fulfilledConditions;
            }
            else
                fulfilledConditions = 1;
        }
        
        if(currentCondition->type == (EVENTCONDITION_STATE & flags)  )
        {
            if(p->currentState != atom_getlong(&currentCondition->values[0]))
            {
                fulfilledConditions = 0;
                return fulfilledConditions;
            }
            else
                fulfilledConditions = 1;
        }
        
        if(  currentCondition->type == (EVENTCONDITION_ANYMESSAGE & flags)  )
        {
            
            if(message != atom_getsym(&currentCondition->values[0]))
            {
                fulfilledConditions = 0;
                return fulfilledConditions;
            }
            else
            {
                fulfilledConditions = 1;
            }
        }
        
        if(currentCondition->type == (EVENTCONDITION_NUMBEROFSTEPS & flags)  )
        {
            if(p->currentNumberOfSteps < atom_getlong(&currentCondition->values[0]))
            {
                fulfilledConditions = 0;
                return fulfilledConditions;
            }
            else
                fulfilledConditions = 1;
        }
        
        if(currentCondition->type == (EVENTCONDITION_NUMBEROFSTEPSINCURRENTSTATE & flags)  )
        {
            if( (p->currentNumberOfSteps-p->lastNumberOfSteps) < atom_getlong(&currentCondition->values[0]))
            {
                fulfilledConditions = 0;
                return fulfilledConditions;
            }
            else
                fulfilledConditions = 1;
        }
        
        if(currentCondition->type == (EVENTCONDITION_FLAGS & flags) )
        {
            if(!p->flags[atom_getlong(&currentCondition->values[0])])
            {
                fulfilledConditions = 0;
                return fulfilledConditions;
            }
            else
                fulfilledConditions = 1;
        }
        
        if(currentCondition->type == (EVENTCONDITION_NOFLAGS & flags) )
        {
            if(p->flags[atom_getlong(&currentCondition->values[0])])
            {
                fulfilledConditions = 0;
                return fulfilledConditions;
            }
            else
                fulfilledConditions = 1;
        }
        
        if(currentCondition->type == (EVENTCONDITION_CLOSEBY & flags) )
        {
            if(!fhnwAudiowalkState_utility_withinDistance(x, 0L, 2, &currentCondition->values[0], p) )
            {
                fulfilledConditions = 0;
                return fulfilledConditions;
            }
            else
                fulfilledConditions = 1;
        }
        
        if(currentCondition->type == (EVENTCONDITION_POSITIONCHANGED & flags) )
        {
            distance = sqrt( (pow((p->currentPosition.x - p->lastPosition.x), 2)) + (pow((p->currentPosition.y - p->lastPosition.y), 2)) );
            distance2check = atom_getfloat(&currentCondition->values[0]);
            
            if(distance < distance2check )
            {
                fulfilledConditions = 0;
                return fulfilledConditions;
            }
            else
                fulfilledConditions = 1;
        }
        
        if(currentCondition->type == (EVENTCONDITION_NOTPOSITIONCHANGED & flags) )
        {
            distance = sqrt( (pow((p->currentPosition.x - p->lastPosition.x), 2)) + (pow((p->currentPosition.y - p->lastPosition.y), 2)) );
            distance2check = atom_getfloat(&currentCondition->values[0]);
            
            if(distance >= distance2check )
            {
                fulfilledConditions = 0;
                return fulfilledConditions;
            }
            else
                fulfilledConditions = 1;
        }
        
        if(currentCondition->type == (EVENTCONDITION_ELAPSEDTIMEINCURRENTSTATE & flags) )
        {
            if( (x->overallTime-p->startTimeInCurrentState) < atom_getfloat(&currentCondition->values[0]))
            {
                fulfilledConditions = 0;
                return fulfilledConditions;
            }
            else
                fulfilledConditions = 1;
        }
        
        if(currentCondition->type == (EVENTCONDITION_TIMERANGEINCURRENTSTATE & flags)  )
        {
            if( !((x->overallTime-p->startTimeInCurrentState) >= atom_getfloat(&currentCondition->values[0]) && (x->overallTime-p->startTimeInCurrentState) < atom_getfloat(&currentCondition->values[1])) )
            {
                fulfilledConditions = 0;
                return fulfilledConditions;
            }
            else
                fulfilledConditions = 1;
        }
        
        if(currentCondition->type == (EVENTCONDITION_ELAPSEDTIMEINCURRENTSTATE_TIMEOUT & flags)  ) // timeOut is counted in numberOfTimeOuts
        {
            if( (x->overallTime-p->startTimeInCurrentState) < atom_getfloat(&currentCondition->values[0]) || (p->currentState == p->lastState) )
            {
                fulfilledConditions = 0;
                return fulfilledConditions;
            }
            else
            {
                fulfilledConditions = 1;
                maxTimeCount = 1;
            }
        }
        
        if(currentCondition->type == (EVENTCONDITION_ELAPSEDOVERALLTIME & flags)  )
        {
            if( (x->overallTime-p->startOverallTime) < atom_getfloat(&currentCondition->values[0]) || (p->currentState == p->lastState)  )
            {
                fulfilledConditions = 0;
                return fulfilledConditions;
            }
            else
                fulfilledConditions = 1;
        }
        
        if(currentCondition->type == (EVENTCONDITION_YES & flags)  )
        {
            if(message != gensym("yes"))
            {
                fulfilledConditions = 0;
                return fulfilledConditions;
            }
            else
            {
                fulfilledConditions = 1;
                yes = 1;
            }
        }
        
        if(currentCondition->type == (EVENTCONDITION_NO & flags)  )
        {
            if(message != gensym("no"))
            {
                fulfilledConditions = 0;
                return fulfilledConditions;
            }
            else
            {
                fulfilledConditions = 1;
                no = 1;
            }
        }
        
        if(currentCondition->type == (EVENTCONDITION_GETUP & flags)  )
        {
            if(message != gensym("getUp"))
            {
                fulfilledConditions = 0;
                return fulfilledConditions;
            }
            else
            {
                fulfilledConditions = 1;
                
            }
        }
        
        if(currentCondition->type == (EVENTCONDITION_SITDOWN & flags)  )
        {
            if(message != gensym("sitDown"))
            {
                fulfilledConditions = 0;
                return fulfilledConditions;
            }
            else
            {
                fulfilledConditions = 1;
            }
        }
        
        if(currentCondition->type == (EVENTCONDITION_NUMBEROFNO & flags)  )
        {
            if(p->numberOfNo != atom_getlong(&currentCondition->values[0]))
            {
                fulfilledConditions = 0;
                return fulfilledConditions;
            }
            else
                fulfilledConditions = 1;
        }
        
        if(currentCondition->type == (EVENTCONDITION_NUMBEROFTIMEOUT & flags)  )
        {
            if(p->numberOfTimeOut < atom_getlong(&currentCondition->values[0])) // größer oder kleiner??
            {
                fulfilledConditions = 0;
                return fulfilledConditions;
            }
            else
                fulfilledConditions = 1;
        }
        
        if(currentCondition->type == (EVENTCONDITION_COMPLY & flags )  )
        {
            if(message != gensym("comply"))
            {
                fulfilledConditions = 0;
                return fulfilledConditions;
            }
            else
            {
                fulfilledConditions = 1;
                comply = 1;
            }
        }
        
        if(currentCondition->type == (EVENTCONDITION_NOCOMPLY & flags ) )
        {
            if(message != gensym("noComply"))
            {
                fulfilledConditions = 0;
                return fulfilledConditions;
            }
            else
                fulfilledConditions = 1;
        }
        
        if(currentCondition->type == (EVENTCONDITION_POSITION & flags)  )
        {
            position2check.x = atom_getfloat(&currentCondition->values[0]);
            position2check.y = atom_getfloat(&currentCondition->values[1]);
            position2check.width = atom_getfloat(&currentCondition->values[2]);
            position2check.height = atom_getfloat(&currentCondition->values[3]);
            
            if(!fhnwAudiowalkState_utility_pointWithinRect(p->currentPosition, position2check))
            {
                fulfilledConditions = 0;
                return fulfilledConditions;
            }
            else
            {
                position = 1;
                fulfilledConditions = 1;
            }
        }
        
        if(currentCondition->type == (EVENTCONDITION_ORIENTATION & flags)  )
        {
            orientation2check = atom_getlong(&currentCondition->values[0]);
            orientation2checkRange = atom_getlong(&currentCondition->values[1]);
            
            if( !( (p->currentOrientation >= orientation2check) && (p->currentOrientation < orientation2checkRange) ) )
            {
                fulfilledConditions = 0;
                return fulfilledConditions;
            }
            else
            {
                orientation = 1;
                fulfilledConditions = 1;
            }
        }
        
        if(currentCondition->type == (EVENTCONDITION_ORIENTATIONCHANGED & flags)  )
        {
            orientation2check = atom_getlong(&currentCondition->values[0]);
            
            if( ( abs(p->currentOrientation - p->lastOrientation) < orientation2check) || // this is not correct, should be done in radians...
               ( abs(p->currentOrientation - (p->lastOrientation + 360)) < orientation2check ) )
            {
                fulfilledConditions = 0;
                return fulfilledConditions;
            }
            else
            {
                orientation = 1;
                fulfilledConditions = 1;
            }
        }
        
        if(currentCondition->type == (EVENTCONDITION_NOTORIENTATIONCHANGED & flags)  )
        {
            orientation2check = atom_getlong(&currentCondition->values[0]);
            
            if( !( ( abs(p->currentOrientation - p->lastOrientation) < orientation2check) || // same
                  ( abs(p->currentOrientation - (p->lastOrientation + 360)) < orientation2check ) ) )
            {
                fulfilledConditions = 0;
                return fulfilledConditions;
            }
            else
            {
                orientation = 1;
                fulfilledConditions = 1;
            }
        }
        
        if(currentCondition->type == (EVENTCONDITION_ROTATION & flags)  )
        {
            orientation2check = atom_getlong(&currentCondition->values[0]);
            
            if( p->rotation != orientation2check)
            {
                fulfilledConditions = 0;
                return fulfilledConditions;
            }
            else
            {
                orientation = 1;
                fulfilledConditions = 1;
            }
        }
        
        if(currentCondition->type == (EVENTCONDITION_WALKINGDIRECTION & flags)  )
        {
            orientation2check = atom_getlong(&currentCondition->values[0]);
            if(orientation2check == 1)
            {
                if(!fhnwAudiowalkState_isWalkingForward(p->currentOrientation, p->walkingDirection))
                {
                    fulfilledConditions = 0;
                    return fulfilledConditions;
                }
                else
                    fulfilledConditions = 1;
            }
            
            else if(orientation2check == 2 || orientation2check == -1)
            {
                if(fhnwAudiowalkState_isWalkingForward(p->currentOrientation, p->walkingDirection))
                {
                    fulfilledConditions = 0;
                    return fulfilledConditions;
                }
                else
                    fulfilledConditions = 1;
            }
            else
            {
                fulfilledConditions = 0;
                return fulfilledConditions;
            }
        }
        
        if(currentCondition->type == (EVENTCONDITION_NOTPOSITION & flags)  )
        {
            position2check.x = atom_getfloat(&currentCondition->values[0]);
            position2check.y = atom_getfloat(&currentCondition->values[1]);
            position2check.width = atom_getfloat(&currentCondition->values[2]);
            position2check.height = atom_getfloat(&currentCondition->values[3]);
            
            if(fhnwAudiowalkState_utility_pointWithinRect(p->currentPosition, position2check))
            {
                fulfilledConditions = 0;
                return fulfilledConditions;
            }
            else
            {
                position = 1;
                fulfilledConditions = 1;
            }
        }
        
        linklist_next(e->conditions, (void *)currentCondition, (void *)&nextCondition);
        currentCondition = nextCondition;
    }
    
    // if all conditions for a state transition are fulfied, we count the numberOf... stuff
    
    if(maxTimeCount)
    {
        p->numberOfTimeOut++;
        //post("numberOfTimeOut: %s %d", p->name->s_name, p->numberOfTimeOut);
    }
    if(no)
    {
        p->numberOfNo++;
        //post("numberOfNo: %s %d", p->name->s_name, p->numberOfNo);
    }
    if(yes)
    {
        p->numberOfYes++;
        //post("numberOfYes: %s %d", p->name->s_name, p->numberOfYes);
    }
    if(comply)
    {
        p->numberOfComply++;
        //post("numberOfComply: %s %d", p->name->s_name, p->numberOfComply);
    }
    
    p->lastPosition.x = p->currentPosition.x; p->lastPosition.y = p->currentPosition.y;
    p->lastOrientation = p->currentOrientation;
    p->lastNumberOfSteps = p->currentNumberOfSteps;
    
    return fulfilledConditions;
}

//--------------------------------------------------------------------------------------------------------------------------------------//
//----------------------- fhnwAudiowalkState_processEvent -----------------------------------------------------------------------------------------//
//----------------------- called from fhnwAudiowalkState_tick every 250ms and by the process message ----------------------------------------------//
//------------------------checks whether a client fulfills all conditions in order to move to new state --------------------------------//
//--------------------------------------------------------------------------------------------------------------------------------------//

void fhnwAudiowalkState_processEvent(t_fhnwAudiowalkState *x, t_symbol *message, short argc, t_atom *argv)
{
    fhnwAudiowalkState_event *currentEvent, *nextEvent;
    fhnwAudiowalkState_client *currentClient, *nextClient;
    
    int fulfilledConditions = 0;
    long flags = 0;
    char intName[10];
    
    t_symbol *clientName = NULL;
    t_symbol *clientMessage;
    
    if(!x->onOff)
        return;
    
    //flags can be optimized for scheduler and process message
    
    flags = 0
    | EVENTCONDITION_CLIENTID
    | EVENTCONDITION_POSITION
    | EVENTCONDITION_NOTPOSITION
    | EVENTCONDITION_POSITIONCHANGED
    | EVENTCONDITION_NOTPOSITIONCHANGED
    | EVENTCONDITION_ANYMESSAGE
   // | EVENTCONDITION_CLOSEBY
    | EVENTCONDITION_STATE
    | EVENTCONDITION_ELAPSEDTIMEINCURRENTSTATE
    | EVENTCONDITION_TIMERANGEINCURRENTSTATE
    | EVENTCONDITION_ELAPSEDTIMEINCURRENTSTATE_TIMEOUT
    | EVENTCONDITION_ELAPSEDOVERALLTIME
    | EVENTCONDITION_FLAGS
    | EVENTCONDITION_NOFLAGS
    | EVENTCONDITION_YES
    | EVENTCONDITION_NO
    | EVENTCONDITION_COMPLY
    | EVENTCONDITION_NOCOMPLY
    | EVENTCONDITION_NUMBEROFNO
    | EVENTCONDITION_NUMBEROFYES
    | EVENTCONDITION_NUMBEROFTIMEOUT
    | EVENTCONDITION_ORIENTATION
    | EVENTCONDITION_ROTATION
    | EVENTCONDITION_NOTORIENTATION
    | EVENTCONDITION_ORIENTATIONCHANGED
    | EVENTCONDITION_NOTORIENTATIONCHANGED
   // | EVENTCONDITION_SITDOWN
   // | EVENTCONDITION_GETUP
    | EVENTCONDITION_WALKINGDIRECTION
    | EVENTCONDITION_NUMBEROFSTEPSINCURRENTSTATE
    | EVENTCONDITION_NUMBEROFSTEPS
    ;
    
    //------------------------------------- check position, orientation, time and flags for all clients every 0.25 seconds---------------------------//
    
    if(argc == 0)
    {
        currentClient = linklist_getindex(x->clientList, 0);
        while(currentClient != NULL)
        {
            if(!currentClient->inWaitMode && currentClient->active)
            {
                currentEvent = linklist_getindex(x->eventList, 0);
                
                while(currentEvent != NULL)
                {
                    fulfilledConditions = 0;
                    fulfilledConditions = fhnwAudiowalkState_checkConditions(x, NULL, currentClient, currentEvent, flags);
                    
                    if(fulfilledConditions)
                        break;
                    
                    linklist_next(x->eventList, (void *)currentEvent, (void *)&nextEvent);
                    currentEvent = nextEvent;
                }
                
                if(fulfilledConditions == 1)
                    fhnwAudiowalkState_processStateAndMessagesAndFlags(x,  0L, currentClient, currentEvent);
            }
            
            linklist_next(x->clientList, (void *)currentClient, (void *)&nextClient);
            currentClient = nextClient;
        }
    }
    
    //------------------------------------- individual events with persID, yes no comply no comply ------------------------------------//
    
    else if(argc == 2)  // YES, NO, COMPLY, NOCOMPLY, GETUP, SITDOWN, ANYMESSAGE
    {
        if(atom_gettype(argv) == A_SYM)
            clientName = atom_getsym(argv);
        else if(atom_gettype(argv) == A_LONG)
        {
            sprintf(intName, "%d", (int)atom_getlong(argv));
            clientName = gensym(intName);
        }
        else
            return;
        
        if(atom_gettype(argv+1) == A_SYM)
            clientMessage = atom_getsym(argv+1);
        
        /*else if(atom_gettype(argv+1) == A_LONG) // long is incomplete and not in use
        {
            if(atom_getlong(argv+1) == 1)
                clientMessage = gensym("yes");
            else if(atom_getlong(argv+1) == 2)
                clientMessage = gensym("no");
            else if(atom_getlong(argv+1) == 3)
                clientMessage = gensym("sitDown");
            else if(atom_getlong(argv+1) == 4)
                clientMessage = gensym("getUp");
            else
                return;
        }*/
        else
            return;
        
        if(clientName)
        {
            
            currentClient = linklist_getindex(x->clientList, 0);
            while(currentClient != NULL)
            {
                if((clientName == currentClient->name) && !currentClient->inWaitMode && currentClient->active)
                {
                    currentEvent = linklist_getindex(x->eventList, 0);
                    while(currentEvent != NULL)
                    {
                        fulfilledConditions = 0;
                        fulfilledConditions = fhnwAudiowalkState_checkConditions(x, clientMessage, currentClient, currentEvent, flags);
                        
                        if(fulfilledConditions)
                            break;
                        
                        linklist_next(x->eventList, (void *)currentEvent, (void *)&nextEvent);
                        currentEvent = nextEvent;
                    }
                    
                    if(fulfilledConditions)
                    {
                        fhnwAudiowalkState_processStateAndMessagesAndFlags(x,  0L, currentClient, currentEvent);
                        //send by a specific client, so break the loop here
                        break;
                    }
                }
                
                linklist_next(x->clientList, (void *)currentClient, (void *)&nextClient);
                currentClient = nextClient;
            }
        }
    }
    else;
}

//--------------------------------------------------------------------------------------------------------------------------------------//
//----------------------- fhnwAudiowalkState_position ---------------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------------------------//

void fhnwAudiowalkState_position(t_fhnwAudiowalkState *x, t_symbol *message, short argc, t_atom *argv)
{
    fhnwAudiowalkState_client *currentClient, *nextClient;
    t_symbol *clientName = NULL;
    char intName[10];
    
    if(!x->onOff)
        return;
    
    if(argc == 3)
    {
        if(atom_gettype(argv) == A_SYM)
            clientName = atom_getsym(argv);
        else if(atom_gettype(argv) == A_LONG)
        {
            sprintf(intName, "%d", (int)atom_getlong(argv));
            clientName = gensym(intName);
        }
        else
            return;
        
        if(atom_gettype(argv+1) != A_FLOAT)
            return;
        if(atom_gettype(argv+2) != A_FLOAT)
            return;
        
        if(clientName)  // message send by a client, clientID's start at 1
        {
            currentClient = linklist_getindex(x->clientList, 0);
            while(currentClient != NULL)
            {
                linklist_next(x->clientList, (void *)currentClient, (void *)&nextClient);
                if(clientName == currentClient->name)
                    break;
                else
                    currentClient = nextClient;
            }
            
            if(currentClient == NULL)  // passed invalid ID
                return;
            
            currentClient->currentPosition.x = atom_getfloat(argv+1);
            currentClient->currentPosition.y = atom_getfloat(argv+2);
        }
    }
    else;
}

//--------------------------------------------------------------------------------------------------------------------------------------//
//----------------------- fhnwAudiowalkState_step -------------------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------------------------//

void fhnwAudiowalkState_step(t_fhnwAudiowalkState *x, t_symbol *message, short argc, t_atom *argv)
{
    fhnwAudiowalkState_client *currentClient, *nextClient;
    t_symbol *clientName = NULL;
    char intName[10];
    
    if(!x->onOff)
        return;
    
    if(argc == 2)
    {
        if(atom_gettype(argv) == A_SYM)
            clientName = atom_getsym(argv);
        else if(atom_gettype(argv) == A_LONG)
        {
            sprintf(intName, "%d", (int)atom_getlong(argv));
            clientName = gensym(intName);
        }
        else
            return;
        
        if(atom_gettype(argv+1) != A_LONG)
            return;
        
        if(clientName)  // message send by a client, clientID's start at 1
        {
            currentClient = linklist_getindex(x->clientList, 0);
            while(currentClient != NULL)
            {
                linklist_next(x->clientList, (void *)currentClient, (void *)&nextClient);
                if(clientName == currentClient->name)
                    break;
                else
                    currentClient = nextClient;
            }
            
            if(currentClient == NULL)  // passed invalid ID
                return;
            
            currentClient->currentNumberOfSteps = atom_getlong(argv+1);
        }
    }
    else;
}

//--------------------------------------------------------------------------------------------------------------------------------------//
//----------------------- fhnwAudiowalkState_rotation ---------------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------------------------//

void fhnwAudiowalkState_rotation(t_fhnwAudiowalkState *x, t_symbol *message, short argc, t_atom *argv)
{
    fhnwAudiowalkState_client *currentClient, *nextClient;
    t_symbol *clientName = NULL;
    char intName[10];
    
    if(!x->onOff)
        return;
    
    if(argc == 2)
    {
        if(atom_gettype(argv) == A_SYM)
            clientName = atom_getsym(argv);
        else if(atom_gettype(argv) == A_LONG)
        {
            sprintf(intName, "%d", (int)atom_getlong(argv));
            clientName = gensym(intName);
        }
        else
            return;
        
        if(atom_gettype(argv+1) != A_LONG)
            return;
        
        if(clientName)  // message send by a client, clientID's start at 1
        {
            currentClient = linklist_getindex(x->clientList, 0);
            while(currentClient != NULL)
            {
                linklist_next(x->clientList, (void *)currentClient, (void *)&nextClient);
                if(clientName == currentClient->name)
                    break;
                else
                    currentClient = nextClient;
            }
            
            if(currentClient == NULL)  // passed invalid ID
                return;
         
             currentClient->rotation = atom_getlong(argv+1);
        }
    }
}

//--------------------------------------------------------------------------------------------------------------------------------------//
//----------------------- fhnwAudiowalkState_orientation ------------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------------------------//

void fhnwAudiowalkState_orientation(t_fhnwAudiowalkState *x, t_symbol *message, short argc, t_atom *argv)
{
    fhnwAudiowalkState_client *currentClient, *nextClient;
    t_symbol *clientName = NULL;
    char intName[10];
    
    if(!x->onOff)
        return;
    
    if(argc == 2)
    {
        if(atom_gettype(argv) == A_SYM)
            clientName = atom_getsym(argv);
        else if(atom_gettype(argv) == A_LONG)
        {
            sprintf(intName, "%d", (int)atom_getlong(argv));
            clientName = gensym(intName);
        }
        else
            return;
        
        if(atom_gettype(argv+1) != A_LONG)
            return;
        
        if(clientName)  // message send by a client, clientID's start at 1
        {
            currentClient = linklist_getindex(x->clientList, 0);
            while(currentClient != NULL)
            {
                linklist_next(x->clientList, (void *)currentClient, (void *)&nextClient);
                if(clientName == currentClient->name)
                    break;
                else
                    currentClient = nextClient;
            }
            
            if(currentClient == NULL)  // passed invalid ID
                return;
            
            currentClient->currentOrientation = atom_getlong(argv+1);
            // currentClient->rotation = atom_getlong(argv+2);
            
        }
    }
    
    else if(argc == 3)// position server includes locality id, not used by fhnw.audiowalk.state since gate events are responsible for separating rooms
    {
        if(atom_gettype(argv) == A_SYM)
            clientName = atom_getsym(argv);
        else if(atom_gettype(argv) == A_LONG)
        {
            sprintf(intName, "%d", (int)atom_getlong(argv));
            clientName = gensym(intName);
        }
        else
            return;
        
        if(atom_gettype(argv+2) == A_SYM)
            return;
        
        if(clientName)  // message send by a client, clientID's start at 1
        {
            currentClient = linklist_getindex(x->clientList, 0);
            while(currentClient != NULL)
            {
                linklist_next(x->clientList, (void *)currentClient, (void *)&nextClient);
                if(clientName == currentClient->name)
                    break;
                else
                    currentClient = nextClient;
            }
            
            if(currentClient == NULL)  // passed invalid ID
                return;
            
            if(atom_gettype(argv+2) == A_LONG)
                currentClient->currentOrientation = atom_getlong(argv+2);
            else if(atom_gettype(argv+2) == A_FLOAT)
                currentClient->currentOrientation = atom_getfloat(argv+2);
                
           // currentClient->rotation = atom_getlong(argv+2);
            
        }
    }
    else;
}

void fhnwAudiowalkState_free(t_fhnwAudiowalkState *x)
{
    linklist_clear(x->eventList);
    linklist_clear(x->clientList);
    clock_free(x->timer);
}

//--------------------------------------------------------------------------------------------------------------------------------------//
//----------------------- fhnwAudiowalkState_write ------------------------------------------------------------------------------------------------//
//------------------------not finished and not in use since saving is realized with a max coll object ----------------------------------//
//--------------------------------------------------------------------------------------------------------------------------------------//

void fhnwAudiowalkState_write(t_fhnwAudiowalkState *x, t_symbol *message, short argc, t_atom *argv)
{
    fhnwAudiowalkState_event *currentEvent, *nextEvent;
    fhnwAudiowalkState_condition *currentCondition, *nextCondition;
    fhnwAudiowalkState_message *currentMessage, *nextMessage;
    fhnwAudiowalkState_client *p, *nextClient;
    t_fourcc filetype = 'TEXT', outtype;
    
    t_handle h;
    t_filehandle fh;
    t_atom out[40];
    
    long size = 512;
    
    long err;
    
    int i = 0;
    int j = 0;
    int writeIndex = 1;
    
    short path;
    char filename[MAX_FILENAME_CHARS];
    char **write;
    char writeIndexChar[572];
    
    write = (char **) sysmem_newptr(sizeof(char *));
    *write = (char *) sysmem_newptr(512);
    
    strcpy(filename, "untitled.txt");
    if (saveasdialog_extended(filename, &path, &outtype, &filetype, 1))     // non-zero: user cancelled
        return;
    
    h = sysmem_newhandle(0);
    err = path_createsysfile(filename, path, 'TEXT', &fh);
    
    if (err)
        return;
    
    currentEvent = linklist_getindex(x->eventList, 0);
    while(currentEvent != NULL)
    {
        i=0;
        sprintf(writeIndexChar, "%d, ", writeIndex);
        strcat(writeIndexChar, "\0");
        
        atom_setsym(out+i, gensym("newEvent"));
        i++;
        currentCondition = linklist_getindex(currentEvent->conditions, 0);
        
        atom_setsym(out+i, gensym("nextState"));
        i++;
        atom_setlong(out+i, currentEvent->nextState);
        i++;
        
        if(currentEvent->flag2set >= 0)
        {
            atom_setsym(out+i, gensym("setFlag"));
            i++;
            atom_setlong(out+i,currentEvent->flag2set);
            i++;
        }
        
        if(currentEvent->flag2unset >= 0)
        {
            atom_setsym(out+i, gensym("unsetFlag"));
            i++;
            atom_setlong(out+i,currentEvent->flag2unset);
            i++;
        }
        
        while(currentCondition != NULL)
        {
            if(currentCondition->type == EVENTCONDITION_CLIENTID)
            {
                atom_setsym(out+i, gensym("ID"));
                i++;
                atom_setlong(out+i, atom_getlong(&currentCondition->values[0]));
                i++;
            }
            
            if(currentCondition->type == EVENTCONDITION_STATE)
            {
                atom_setsym(out+i, gensym("state"));
                i++;
                atom_setlong(out+i, atom_getlong(&currentCondition->values[0]));
                i++;
            }
            
            if(currentCondition->type == EVENTCONDITION_POSITION)
            {
                atom_setsym(out+i, gensym("position"));
                i++;
                for(j=0;j<4;j++)
                {
                    atom_setfloat(out+i, atom_getfloat(&currentCondition->values[j]));
                    i++;
                }
            }
            if(currentCondition->type == EVENTCONDITION_YES)
            {
                atom_setsym(out+i, gensym("yes"));
                i++;
            }
            
            if(currentCondition->type == EVENTCONDITION_NO)
            {
                atom_setsym(out+i, gensym("no"));
                i++;
            }
            
            if(currentCondition->type == EVENTCONDITION_COMPLY)
            {
                atom_setsym(out+i, gensym("comply"));
                i++;
            }
            
            if(currentCondition->type == EVENTCONDITION_NOCOMPLY)
            {
                atom_setsym(out+i, gensym("noComply"));
                i++;
            }
            
            if(currentCondition->type == EVENTCONDITION_ELAPSEDTIMEINCURRENTSTATE)
            {
                atom_setsym(out+i, gensym("maxTime"));
                i++;
                atom_setfloat(out+i, atom_getfloat(&currentCondition->values[0]));
                i++;
            }
            
            if(currentCondition->type == EVENTCONDITION_ELAPSEDOVERALLTIME)
            {
                atom_setsym(out+i, gensym("maxOverallTime"));
                i++;
                atom_setfloat(out+i, atom_getfloat(&currentCondition->values[0]));
                i++;
            }
            
            if(currentCondition->type == EVENTCONDITION_FLAGS)
            {
                atom_setsym(out+i, gensym("flag"));
                i++;
                atom_setlong(out+i, atom_getlong(&currentCondition->values[0]));
                i++;
            }
            
            if(currentCondition->type == EVENTCONDITION_NOFLAGS)
            {
                atom_setsym(out+i, gensym("noFlag"));
                i++;
                atom_setlong(out+i, atom_getlong(&currentCondition->values[0]));
                i++;
            }
            
            linklist_next(currentEvent->conditions, (void *)currentCondition, (void *)&nextCondition);
            currentCondition = nextCondition;
        }
        
        currentMessage = linklist_getindex(currentEvent->messages, 0);
        while(currentMessage != NULL)
        {
            atom_setsym(out+i, gensym("message"));
            i++;
            
            atom_setsym(out+i, currentMessage->target);
            i++;
            
            atom_setsym(out+i, currentMessage->message);
            i++;
            
            linklist_next(currentEvent->messages, (void *)currentMessage, (void *)&nextMessage);
            currentMessage = nextMessage;
        }
        
        atom_gettext(i, out, &size, write, OBEX_UTIL_ATOM_GETTEXT_DEFAULT);
        
        strcat(writeIndexChar, *write);
        strcat(writeIndexChar, ";");
        strcat(writeIndexChar, "\n");
        sysmem_ptrandhand(writeIndexChar, h,strlen(writeIndexChar));
        writeIndex++;
        memset(*write, 512, 0);
        linklist_next(x->eventList, (void *)currentEvent, (void *)&nextEvent);
        currentEvent = nextEvent;
    }
    
    p = linklist_getindex(x->clientList, 0);
    
    while(p != NULL)
    {
        i=0;
        sprintf(writeIndexChar, "%d, ", writeIndex);
        strcat(writeIndexChar, "\0");
        
        atom_setsym(out+i, gensym("newClient"));
        i++;
        
        atom_setsym(out+i, p->name);
        i++;
        
        atom_gettext(i, out, &size, write, OBEX_UTIL_ATOM_GETTEXT_DEFAULT);
        strcat(writeIndexChar, *write);
        strcat(writeIndexChar, ";");
        strcat(writeIndexChar, "\n");
        sysmem_ptrandhand(writeIndexChar, h,strlen(writeIndexChar));
        memset(*write, 512, 0);
        writeIndex++;
        linklist_next(x->clientList, (void *)p, (void *)&nextClient);
        p = nextClient;
    }
    
    err = sysfile_writetextfile(fh, h, TEXT_LB_NATIVE);
    sysfile_close(fh);
    sysmem_freehandle(h);
    sysmem_freeptr(*write);
    sysmem_freeptr(write);
}

void fhnwAudiowalkState_write_defer(t_fhnwAudiowalkState *x, t_symbol *message, short argc, t_atom *argv)
{
    defer(x, (method) fhnwAudiowalkState_write, message, argc, argv);
}

//--------------------------------------------------------------------------------------------------------------------------------------//
//----------------------- fhnwAudiowalkState_dump -------------------------------------------------------------------------------------------------//
//------------------------not finished and not in use ----------------------------------------------- ----------------------------------//
//--------------------------------------------------------------------------------------------------------------------------------------//

void fhnwAudiowalkState_dump(t_fhnwAudiowalkState *x, t_symbol *message, short argc, t_atom *argv)
{
    int i = 0;
    int j = 0;
    t_atom out[40];
    fhnwAudiowalkState_event *currentEvent, *nextEvent;
    fhnwAudiowalkState_condition *currentCondition, *nextCondition;
    fhnwAudiowalkState_message *currentMessage, *nextMessage;
    
    currentEvent = linklist_getindex(x->eventList, 0);
    while(currentEvent != NULL)
    {
        i = 0;
        atom_setlong(out+i, currentEvent->ID);
        i++;
        currentCondition = linklist_getindex(currentEvent->conditions, 0);
        while(currentCondition != NULL)
        {
            /*if(currentCondition->type == EVENTCONDITION_CLIENTID)
             {
             atom_setsym(out+i, gensym("ID"));
             i++;
             atom_setsym(out+i, atom_getsym(&currentCondition->values[0]));
             i++;
             }*/
            
            if(currentCondition->type == EVENTCONDITION_STATE)
            {
                atom_setsym(out+i, gensym("state"));
                i++;
                atom_setlong(out+i, atom_getlong(&currentCondition->values[0]));
                i++;
            }
            
            if(currentCondition->type == EVENTCONDITION_POSITION)
            {
                atom_setsym(out+i, gensym("position"));
                i++;
                for(j=0;j<4;j++)
                {
                    atom_setfloat(out+i, atom_getfloat(&currentCondition->values[j]));
                    i++;
                }
            }
            
            linklist_next(currentEvent->conditions, (void *)currentCondition, (void *)&nextCondition);
            currentCondition = nextCondition;
        }
        
        currentMessage = linklist_getindex(currentEvent->messages, 0);
        while(currentMessage != NULL)
        {
            atom_setsym(out+i, gensym("message"));
            i++;
            
            atom_setsym(out+i, currentMessage->target);
            i++;
            
            atom_setsym(out+i, currentMessage->message);
            i++;
            
            linklist_next(currentEvent->messages, (void *)currentMessage, (void *)&nextMessage);
            currentMessage = nextMessage;
        }
        
        outlet_list(x->outlet, 0L, i, out);
        linklist_next(x->eventList, (void *)currentEvent, (void *)&nextEvent);
        currentEvent = nextEvent;
    }
}

//--------------------------------------------------------------------------------------------------------------------------------------//
//----------------------- fhnwAudiowalkState_noErrors ---------------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------------------------//

void fhnwAudiowalkState_noErrors(t_fhnwAudiowalkState *x, t_symbol *message, short argc, t_atom *argv)
{;} // allows us to write comments into the coll file without generating error messages

//--------------------------------------------------------------------------------------------------------------------------------------//
//----------------------- main ---------------------------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------------------------//

int C74_EXPORT main(void)
{
	t_class *c;
	
	c = class_new("fhnw.audiowalk.state", (method)fhnwAudiowalkState_new, (method)fhnwAudiowalkState_free, (long)sizeof(t_fhnwAudiowalkState),
				  0L, A_GIMME, 0);
	
    class_addmethod(c, (method)fhnwAudiowalkState_assist,			"assist",		A_CANT, 0);
    class_addmethod(c, (method)fhnwAudiowalkState_newClient,			"newClient",		A_GIMME, 0);
    class_addmethod(c, (method)fhnwAudiowalkState_removeClient,			"removeClient",		A_GIMME, 0);
    class_addmethod(c, (method)fhnwAudiowalkState_nextState,			"nextState",		A_GIMME, 0);
    class_addmethod(c, (method)fhnwAudiowalkState_removeEvent,			"removeEvent",		A_GIMME, 0);
    class_addmethod(c, (method)fhnwAudiowalkState_processEvent,			"process",		A_GIMME, 0);
    class_addmethod(c, (method)fhnwAudiowalkState_processEvent,			"processGesture",		A_GIMME, 0);
    class_addmethod(c, (method)fhnwAudiowalkState_position,			"position",		A_GIMME, 0);
    class_addmethod(c, (method)fhnwAudiowalkState_setState,			"setState",		A_GIMME, 0);
    class_addmethod(c, (method)fhnwAudiowalkState_stopWait,			"stopWait",		A_GIMME, 0);
    class_addmethod(c, (method)fhnwAudiowalkState_orientation,			"orientation",		A_GIMME, 0);
    class_addmethod(c, (method)fhnwAudiowalkState_rotation,			"rotation",		A_GIMME, 0);
    class_addmethod(c, (method)fhnwAudiowalkState_step,			"step",		A_GIMME, 0);
    class_addmethod(c, (method)fhnwAudiowalkState_start,			"start",		A_GIMME, 0);
    class_addmethod(c, (method)fhnwAudiowalkState_stop,			"stop",		A_GIMME, 0);
    class_addmethod(c, (method)fhnwAudiowalkState_reset,			"reset",		A_GIMME, 0);
    class_addmethod(c, (method)fhnwAudiowalkState_clear,			"clear",		A_GIMME, 0);
    //class_addmethod(c, (method)fhnwAudiowalkState_dump,			"dump",		A_GIMME, 0);
   // class_addmethod(c, (method)fhnwAudiowalkState_write_defer,			"write",		A_GIMME, 0);
    class_addmethod(c, (method)fhnwAudiowalkState_noErrors,			"anything",		A_GIMME, 0);
    
    CLASS_ATTR_CHAR(c, "debuggingMessages2MaxWindow", 0, t_fhnwAudiowalkState, debuggingMessages2MaxWindow);
    CLASS_ATTR_LABEL(c, "debuggingMessages2MaxWindow", 0, "Post messages to Max window");
    CLASS_ATTR_STYLE(c, "debuggingMessages2MaxWindow", 0, "onoff");
    CLASS_ATTR_SAVE(c, "debuggingMessages2MaxWindow", 0);
    //CLASS_ATTR_SAVE(c, "debuggingMessages2MaxWindow", 0);
    
    CLASS_ATTR_LONG(c, "schedulerInterval", 0, t_fhnwAudiowalkState, schedulerInterval);
    CLASS_ATTR_LABEL(c, "schedulerInterval", 0,  "Scheduler Interval");
    CLASS_ATTR_DEFAULT(c, "schedulerInterval", 0, "250");
    
	class_register(CLASS_BOX, c); /* CLASS_NOBOX */
	fhnwAudiowalkState_class = c;
    post("fhnw.audiowalk.state v0.6");
    post("by Thomas Resch, developed for http://blogs.fhnw.ch/indoortracking/");
    post("©2013, ©2014, University of Music Basel, Electronic Studio & Research and Development, FHNW");
    
	return 0;
}






















