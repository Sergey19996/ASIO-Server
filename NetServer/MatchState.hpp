#ifndef MATCHSTATE_H
#define MATCHSTATE_H


enum class MatchState {

    WAITING,
    MATCHMAKING,
    ROOM,
    INGAME,
    FINISHED
};
enum class PlayerState {
    LOBBY,
    MATCHMAKING,
    ROOM,
    INGAME,
    FINISHED,
    DISCONNECTED
};
enum class MatchMode
{
    LAST_MAN_STANDING,
    RESPAWN
};




#endif // !MATCHSTATE_H
