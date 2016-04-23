# Protocol
Here are all the possible commands that the chat program could send or receive on top of the RMP protocol.

## Joining commands
 * `ADD_ME <NICKNAME>`
 * `PARTICIPANT_UPDATE @<LEADER_NICKNAME>:<LEADER_IP>:<PORT> [<A_NICKNAME>:<A_IP>:<A_PORT> ...]= <JOIN_LEAVE_MESSAGE>` - sent to all chat clients after a change in participants. The `JOIN_LEAVE_MESSAGE` will be of the form "Alice joined on 192.168.5.81:1923" or "Eve left the chat or crashed".
 * `JOIN_FAILURE` - joining user should retry after short time period
 * `LEADER_ID <LEADER_IP>:<PORT>` - joining user should contact the specified leader
 * `JOIN_NICKNAME_FAILURE` - user must pick a new nickname

## Exchanging messages
 * `MESSAGE_REQUEST <SENDER_NICKNAME>= <MESSAGE_PAYLOAD>`
 * `MESSAGE_BROADCAST <MESSAGE_ID> <SENDER_NICKNAME>= <MESSAGE_PAYLOAD>` Where `MESSAGE_ID` is an non-negative integer and comes from the leader's logical clock

## Election commands
 * `START_ELECTION <SENDER_NICKNAME>` - when a non-leader realizes a leader cannot be reached, it broadcasts this out
 * `ELECTION_STOP` - when a node sees that an election has been started by an "inferior" node
 * `ELECTION_VICTORY <SENDER_NICKNAME>` - user is new leader

## Heartbeats (Not implemented)
 * `HEARTBEAT` - from non-leader to leader once every regular interval
