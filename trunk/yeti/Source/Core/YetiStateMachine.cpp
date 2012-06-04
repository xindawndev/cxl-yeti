#include "YetiStateMachine.h"

NAMEBEG

using namespace StateMachine;

//-----------------------------------------------------------------------------
// Box for states which don't declare own Box class.
_EmptyBox _EmptyBox::theEmptyBox;

//-----------------------------------------------------------------------------
// Helper functions for box creation
template <>
void * StateMachine::_createBox< _EmptyBox >( void * & place )
{
    return &_EmptyBox::theEmptyBox;
}

template <>
void StateMachine::_deleteBox< _EmptyBox >( void * & box, void * & place )
{
}

#ifdef MACHO_SNAPSHOTS
template <>
void * StateMachine::_cloneBox< _EmptyBox >( void * other ) 
{
    return &_EmptyBox::theEmptyBox;
}
#endif

//-----------------------------------------------------------------------------
// Implementation for Alias
void Alias::setState( _MachineBase & machine ) const
{
    machine.setPendingState( key()->instanceGenerator( machine ), myInitializer->clone() );
}

//-----------------------------------------------------------------------------
// Implementation for StateSpecification
_StateInstance & _StateSpecification::_getInstance( _MachineBase & machine )
{
    // Look first in machine for existing StateInstance.
    _StateInstance * & instance = machine.getInstance( 0 );
    if ( !instance )
    {
        instance = new _RootInstance( machine, 0 );
    }

    return *instance;
}

void _StateSpecification::_shutdown() 
{
    _myStateInstance.machine().shutdown();
}

void _StateSpecification::_restore( _StateInstance & current )
{
    _myStateInstance.machine().myCurrentState = &current;
}

void _StateSpecification::setState( const Alias & state )
{
    state.setState( _myStateInstance.machine() );
}

#ifdef MACHO_SNAPSHOTS
void _StateSpecification::setState( _StateInstance & current )
{
    _myStateInstance.machine().setPendingState( current, &_theDefaultInitializer );
}
#endif

//-----------------------------------------------------------------------------
// StateInstance implementation
_StateInstance::_StateInstance( _MachineBase & machine, _StateInstance * parent )
: myMachine( machine )
, mySpecification( 0 )
, myHistory( 0 )
, myParent( parent )
, myBox( 0 )
, myBoxPlace( 0 )
{
}

_StateInstance::~_StateInstance() 
{
    if ( myBoxPlace )
    {
        ::operator delete( myBoxPlace );
    }

    delete mySpecification;
}

void _StateInstance::entry( _StateInstance & previous, bool first )
{
    // Only Root has no parent
    if ( !myParent ) return;

    // first entry or previous state is not substate -> perform entry
    if (first || !previous.isChild(*this))
    {
        myParent->entry(previous, false);

        createBox();

        mySpecification->entry();
    }
}

void _StateInstance::exit(_StateInstance & next)
{
    // Only Root has no parent
    if (!myParent)
        return;

    // self transition or next state is not substate -> perform exit
    if (this == &next || !next.isChild(*this))
    {
        mySpecification->exit();

        // EmptyBox should be most common box, so optimize for this case.
        if (myBox != &_EmptyBox::theEmptyBox)
            mySpecification->_deleteBox(*this);

        myParent->exit(next);
    }
}

void _StateInstance::init( bool history ) 
{
    if ( history && myHistory )
    {
        myMachine.setPendingState(*myHistory, &_theDefaultInitializer);
    } 
    else
    {
        mySpecification->init();
    }

    myHistory = 0;
}

#ifdef MACHO_SNAPSHOTS
void _StateInstance::copy(_StateInstance & original) 
{
    if (original.myHistory)
    {
        _StateInstance * history = myMachine.getInstance(original.myHistory->id());
        YETI_ASSERT(history);
        setHistory(history);
    }

    if (original.myBox)
        cloneBox(original.myBox);
}

_StateInstance * _StateInstance::clone(_MachineBase & newMachine)
{
    YETI_ASSERT(!newMachine.getInstance(id()));

    _StateInstance * parent = 0;
    if (myParent)
        // Tell other machine to clone parent first.
        parent = newMachine.createClone(myParent->id(), myParent);

    _StateInstance * clone = create(newMachine, parent);
    return clone;
}
#endif

//-----------------------------------------------------------------------------
// Base class for Machine objects.
_MachineBase::_MachineBase()
: myCurrentState(0)
, myPendingState(0)
, myPendingInit(0)
, myPendingBox(0)	// Deprecated!
, myPendingEvent(0)
{
}

_MachineBase::~_MachineBase() 
{
    YETI_ASSERT(!myPendingInit);

    delete[] myInstances;
    delete myPendingEvent;
}

Alias _MachineBase::currentState() const 
{
    return Alias(myCurrentState->key());
}

void _MachineBase::setState(_StateInstance & instance, _Initializer * init)
{
    setPendingState(instance, init);
    rattleOn();
}

void _MachineBase::setState(const Alias & state) 
{
    state.setState(*this);
    rattleOn();
}

void _MachineBase::start(_StateInstance & instance)
{
    // Start with Root state
    myCurrentState = &_StateSpecification::_getInstance(*this);
    // Then go to state
    setState(instance, &_theDefaultInitializer);
}

void _MachineBase::start(const Alias & state)
{
    // Start with Root state
    myCurrentState = &_StateSpecification::_getInstance(*this);
    // Then go to state
    setState(state);
}

void _MachineBase::shutdown() 
{
    YETI_ASSERT(!myPendingState);

    // Performs exit actions by going to Root (=StateSpecification) state.
    setState(_StateSpecification::_getInstance(*this), &_theDefaultInitializer);

    myCurrentState = 0;
}

void _MachineBase::allocate(unsigned int count)
{
    myInstances = new _StateInstance *[count];
    for (unsigned int i = 0; i < count; ++i)
        myInstances[i] = 0;
}

void _MachineBase::free(unsigned int count) 
{
    // Free from end of list, so that child states are freed first
    unsigned int i = count;
    while (i > 0) {
        --i;
        delete myInstances[i];
        myInstances[i] = 0;
    }
}

// Clear history of state and children.
void _MachineBase::clearHistoryDeep(unsigned int count, const _StateInstance & instance)
{
    for (unsigned int i = 0; i < count; ++i)
    {
        _StateInstance * s = myInstances[i];
        if (s && s->isChild(instance))
            s->setHistory(0);
    }
}

#ifdef MACHO_SNAPSHOTS
void _MachineBase::copy(_StateInstance ** others, unsigned int count)
{
    // Create StateInstance objects
    for (ID i = 0; i < count; ++i)
        createClone(i, others[i]);

    // Copy StateInstance object's state
    for (ID i = 0; i < count; ++i) {
        _StateInstance * state = myInstances[i];
        if (state) {
            YETI_ASSERT(others[i]);
            state->copy(*others[i]);
        }
    }
}

_StateInstance * _MachineBase::createClone(ID id, _StateInstance * original)
{
    _StateInstance * & clone = getInstance(id);

    // Object already created?
    if (!clone && original)
        clone = original->clone(*this);

    return clone;
}
#endif

// Performs a pending state transition.
void _MachineBase::rattleOn() 
{
    YETI_ASSERT(myCurrentState);

    while (myPendingState || myPendingEvent)
    {
        // Loop here because init actions might change state again.
        while ( myPendingState )
        {
#ifndef NDEBUG
            // Entry/Exit actions may not dispatch events: set dummy event.
            if ( !myPendingEvent )
            {
                myPendingEvent = ( _IEventBase * )&myPendingEvent;
            }
#endif

            // Perform exit actions (which exactly depends on new state).
            myCurrentState->exit( *myPendingState );

            // Store history information for previous state now.
            // Previous state will be used for deep history.
            myCurrentState->setHistorySuper( *myCurrentState );

            _StateInstance * previous = myCurrentState;
            myCurrentState = myPendingState;

            // Deprecated!
            if ( myPendingBox )
            {
                myCurrentState->setBox( myPendingBox );
                myPendingBox = 0;
            }

            // Perform entry actions on next state's parents (which exactly depends on previous state).
            myCurrentState->entry( *previous );

            // State transition complete.
            // Clear 'pending' information just now so that setState would assert in exits and entries, but not in init.
            myPendingState = 0;

            // Use initializer to call proper "init" action.
            _Initializer * init = myPendingInit;
            myPendingInit = 0;

            init->execute( *myCurrentState );
            init->destroy();

            YETI_ASSERT( "Init may only transition to proper substates" &&
                ( !myPendingState ||
                ( myPendingState->isChild( *myCurrentState ) && ( myCurrentState != myPendingState ) ) )
                );

#ifndef NDEBUG
            // Clear dummy event if need be
            if (myPendingEvent == (_IEventBase *) &myPendingEvent)
            {
                myPendingEvent = 0;
            }
#endif
        } // while (myPendingState)

        if ( myPendingEvent )
        {
            _IEventBase * event = myPendingEvent;
            myPendingEvent = 0;
            event->dispatch(*myCurrentState);
            delete event;
        }
    } // while (myPendingState || myPendingEvent)
} // rattleOn

//-----------------------------------------------------------------------------
// Implementation for _AdaptingInitializer
Key _AdaptingInitializer::adapt( Key key ) 
{
    ID id = static_cast< _KeyData * >( key )->id;
    const _StateInstance * instance = myMachine.getInstance( id );
    _StateInstance * history = 0;

    if ( instance ) history = instance->history();

    return history ? history->key() : key;
}

//-----------------------------------------------------------------------------
// Singleton initializers.
_DefaultInitializer _theDefaultInitializer;
_HistoryInitializer _theHistoryInitializer;

NAMEEND