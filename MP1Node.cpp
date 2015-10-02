/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Definition of MP1Node class functions.
 **********************************/

#include "MP1Node.h"

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

/**
 * Overloaded Constructor of the MP1Node class
 * You can add new members to the class if you think it
 * is necessary for your logic to work
 */
MP1Node::MP1Node(Member *member, Params *params, EmulNet *emul, Log *log, Address *address) {
	for( int i = 0; i < 6; i++ ) {
		NULLADDR[i] = 0; /* tạo ra chỉ để dùng gán đia chỉ address->addr vào.*/
	}
	this->memberNode = member;/* mô tả cho ta biết thành viện trong hệ thống */
	this->emulNet = emul;	/* định nghĩa 1 mạng mô phỏng */
	this->log = log;
	this->par = params;
	this->memberNode->addr = *address;
}

/**
 * Destructor of the MP1Node class
 */
MP1Node::~MP1Node() {}

/**
 * FUNCTION NAME: recvLoop
 *
 * DESCRIPTION: This function receives message from the network and pushes into the queue // nhận maseeage từ mạng và đẩy vào hàng đợi
 * This function is called by a node to receive messages currently waiting for it
 */
int MP1Node::recvLoop() {/* nhận kết nối từ 1 member khác */
    if ( memberNode->bFailed ) {/*nếu member này chết kết nối ko thành công */
    	return false;
    }
    else {
    	return emulNet->ENrecv(&(memberNode->addr), enqueueWrapper, NULL, 1, &(memberNode->mp1q));//đựa gói tin vào hang đọi.
    }
}

/**
 * FUNCTION NAME: enqueueWrapper		
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue //sắp xếp các message từ Emulnet vào hàng đợi
 */
int MP1Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);/*static bool enqueue(queue<q_elt> *queue, void *buffer, int size)
								 {
									q_elt element(buffer, size);
									queue->emplace(element);
									return true;
								}*/
}

/**
 * FUNCTION NAME: nodeStart
 *
 * DESCRIPTION: This function bootstraps the node
 * 				All initializations routines for a member.
 * 				Called by the application layer.
 */
void MP1Node::nodeStart(char *servaddrstr, short servport) {
    Address joinaddr;
    joinaddr =

 getJoinAddress();//khỏi tạo node đầu tiên 1.0.0.0:0
	
    // Self booting routines
    if( initThisNode(&joinaddr) == -1 )
{
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "init_thisnode failed. Exit.");
#endif
        exit(1);
    }

    if( !introduceSelfToGroup(&joinaddr) ) {
        finishUpThisNode();
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Unable to join self to group. Exiting.");/*void LOG(Address *, const char * str, ...);*/
#endif
        exit(1);
    }

    return;
}

/**
 * FUNCTION NAME: initThisNode
 *
 * DESCRIPTION: Find out who I am and start up
 */
int MP1Node::initThisNode(Address *joinaddr) { /* khỏi tạo giá trị cho node */
	/*
	 * This function is partially implemented and may require changes
	 */
	int id = *(int*)(&memberNode->addr.addr);
	int port = *(short*)(&memberNode->addr.addr[4]);

	memberNode->bFailed = false;
	memberNode->inited = true;
	memberNode->inGroup = false;
    // node is up!
	memberNode->nnb = 0;
	memberNode->heartbeat = 0;
	memberNode->pingCounter = TFAIL;
	memberNode->timeOutCounter = -1;
    initMemberListTable(memberNode);

    return 0;
}

/**
 * FUNCTION NAME: introduceSelfToGroup
 *
 * DESCRIPTION: Join the distributed system
 */
int MP1Node::introduceSelfToGroup(Address *joinaddr) {
	MessageHdr *msg;
#ifdef DEBUGLOG /*ghi ra file dbg.log */
    static char s[1024];
#endif

    if ( 0 == memcmp((char *)&(memberNode->addr.addr), (char *)&(joinaddr->addr), sizeof(memberNode->addr.addr))) {
        // I am the group booter (first process to join the group). Boot up the group
	//so sánh địa chỉ của s1 và s2 giống nhau hay không?
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Starting up group...");
#endif
        memberNode->inGroup = true;
    }
    else {
       size_t msgsize = sizeof(MessageHdr) + sizeof(joinaddr->addr) + sizeof(long) + 1;//lý dó là vì MessageHdr chỉ có type message 
	//ta cần cấu trúc đầy đủ (type,ip,heartbeat).
        msg = (MessageHdr *) malloc(msgsize * sizeof(char));//cấp phát bộ nhớ 

        // create JOINREQ message: format of data is {struct Address myaddr}
       msg->msgType = JOINREQ;//loại msg là jion request
       memcpy((char *)(msg+1), &memberNode->addr.addr, sizeof(memberNode->addr.addr));//địa chỉ IP
       memcpy((char *)(msg+1) + 1 + sizeof(memberNode->addr.addr), &memberNode->heartbeat, sizeof(long));//heartbeat

#ifdef DEBUGLOG
        sprintf(s,"Trying to join...");
        log->LOG(&memberNode->addr, s);
#endif

        // send JOINREQ message to introducer member
        emulNet->ENsend(&memberNode->addr, joinaddr, (char *)msg, msgsize);//vốn dĩ message trong hàng đợi có cấu trúc (type,ip,port)
	//là do ở đây.

        free(msg);
    }

    return 1;

}

/**
 * FUNCTION NAME: finishUpThisNode
 *
 * DESCRIPTION: Wind up this node and clean up state
 */
int MP1Node::finishUpThisNode(){
   /*
    * Your code goes here
    */
}

/**
 * FUNCTION NAME: nodeLoop
 *
 * DESCRIPTION: Executed periodically at each member <thực hiện định kỳ tại mổi node>
 * 		Check your messages in queue and perform membership protocol duties<kiễm tra mg tại hàng đợi>
 */
void MP1Node::nodeLoop() {
    if (memberNode->bFailed) {
    	return;
    }

    // Check my messages
    checkMessages();

    // Wait until you're in the group...
    if( !memberNode->inGroup ) {
    	return;
    }

    // ...then jump in and share your responsibilites!
    nodeLoopOps();

    return;
}

/**
 * FUNCTION NAME: checkMessages
 *
 * DESCRIPTION: Check messages in the queue and call the respective message handler<kiểm tra thôn điệp trong hàng đơi và xử lý nó>
 */
void MP1Node::checkMessages() {
    void *ptr;
    int size;

    // Pop waiting messages from memberNode's mp1q 
    while ( !memberNode->mp1q.empty() ) {
    	ptr = memberNode->mp1q.front().elt;//dữ liệu có dược là do hàm enqueueWrapper v
    	size = memberNode->mp1q.front().size;
    	memberNode->mp1q.pop();
    	recvCallBack((void *)memberNode, (char *)ptr, size);
    }
    return;
}

/**
 * FUNCTION NAME: recvCallBack
 *
 * DESCRIPTION: Message handler for different message types
 */
//mục đích kiễm tra message yêu cầu là gì ?(request,repond..).

bool MP1Node::recvCallBack(void *env, char *data, int size ) {
	/*
	 * Your code goes here
	 */
	//kiễm tra type messag
	int *msgType = (int *)(data);
	printf("msgTYpe: %d\n",*msgType);//type JOINREQ==0,JOINREP==1.
	if (*msgType == JOINREQ)
	{
		//tao cấu trúc message của gói JOINREQ(ip,port,heartbeat)
		int *senderAddress = (int *) (data+4);//JOINREQ Là kiểu int nên size =4byte -> IP=data+4.
		printf("SenderAddress: %d\n",*senderAddress);
		short *senderPort = (short *)(data+8);//lấy giá trị ở vị trí con trỏ data+6.
		printf("POrt: %d\n",*senderPort);
		long *heartbeat = (long *)(data+11);//lấy giá trị ở vị trí con trỏ data+11. vì IP có size=6.
		addOrUpdateEntry(*senderAddress, *senderPort, *heartbeat);//OK.

		Address destinationAddr = getAddress(*senderAddress, *senderPort);
		//2.2 - Create message
		MessageHdr *msg;
		size_t msgsize = sizeof(MessageHdr) 	//4 bytes: Msg Type
																										   					+ sizeof(destinationAddr.addr) //6 bytes: own address
																																								     					+1 			//1 byte: padding
																																	   					+sizeof(long) 	//8 bytes: heartbeat.
																																																																												  					+sizeof(int)  	//4 bytes: size of table
																								 					+memberNode->memberList.size()*14;// 14: IP:6,HB:8(trong IP có port và time + id).
		msg = (MessageHdr *) malloc(msgsize * sizeof(char));
		msg->msgType =  JOINREP;
		memcpy((char *)(msg+1), &memberNode->addr.addr, sizeof(memberNode->addr.addr));

		//memberNode->heartbeat = 66055;
		memcpy((char *)(msg+1) + 1 + sizeof(memberNode->addr.addr), &memberNode->heartbeat, sizeof(long));

		int currentOffset = 4+1+ sizeof(memberNode->addr.addr) + sizeof(long);
		char *ptr = ((char*) msg + currentOffset);
		writeMemberListTable(ptr);

		//3-Send Message
		emulNet->ENsend(&memberNode->addr, &destinationAddr, (char *)msg, msgsize);
		free(msg);
		//free(&destinationAddr);
		printMemberListTable();

	}

	else if (*msgType == JOINREP || *msgType == DUMMYLASTMSGTYPE){
		memberNode->inGroup = true;
		//1 Read Message headers:
		int offset = sizeof(int);
		int *senderAddress = (int *) (data+offset);
		offset += sizeof(int);

		short *senderPort = (short *)(data+offset);
		offset += sizeof(short) + 1; //includes 1 byte pad

		long *heartbeat = (long *)(data+offset);
		offset += sizeof(long);
		addOrUpdateEntry(*senderAddress, *senderPort, *heartbeat);

		int *numberElements = (int *) (data+offset);
		offset += sizeof(int);

		if (*msgType == JOINREP)
			;
		//printf("JoinREP: From: %d:%x. Heartbeat:%lu, Elements:%d\n", *senderAddress, *senderPort, *heartbeat, *numberElements);
		//if (*msgType == HEARTBEAT)
			;//printf("Heartbeat: From: %d:%x. Heartbeat:%lu, Elements:%d\n", *senderAddress, *senderPort, *heartbeat, *numberElements);


		//2 read content of member list
		for (int i = 0; i < *numberElements; i++){

			int *id = (int *) (data + offset);
			offset += sizeof(int);
			short *port = (short *) (data + offset);
			offset += sizeof(short);
			long *hb = (long *) (data + offset);
			offset += sizeof(long);
			//	printf("\tJoinREP Loop %d - %d:%x. Heartbeat:%lu\n", i, *id, *port, *hb);

			addOrUpdateEntry(*id, *port, *hb);
		}


	}
	else {
		printf("Received an unknown kind of message. Ignoring.\n");
	}
	
			
}
/**
 * FUNCTION NAME: addOrUpdateEntry

 *
 * DESCRIPTION: nơi lưu trử tạm thời các message để xử lý

 */
void MP1Node::addOrUpdateEntry(int id, short port, long hb){

	//printf("Adding or updating member: received hb:%lu\n", hb);

	//1-Check if exists is an entry for this id:port
	int found = 0;
	int i = 0;
	while (found == 0 && i <  memberNode->memberList.size()){//memberlist có id,port,heartbeat,longtime.
		int currentId = memberNode->memberList.at(i).id;
		short currentPort = memberNode->memberList.at(i).port;
		if (currentId == id and currentPort == port  ){//kiểm trả IP có tồn tại trong table ko?
			long currentHb =  memberNode->memberList.at(i).heartbeat;//gán hb để so sánh hb.
			if (hb > currentHb && currentHb != -1 ){

				//printf("updating: %d:%x . Hb: was: %lu, new: %lu!\n", id, port,memberNode->memberList.at(i).heartbeat, hb );
				//if eists, update heartbeart and timestamp:
				memberNode->memberList.at(i).heartbeat = hb;//cập nhật lại heartbeat.
				memberNode->memberList.at(i).timestamp = memberNode->heartbeat;//gán laị time tại thời điểm đó.
			}
			found = 1;

		}


		i++;
	}
	//if entry was not found, add it
	if (found == 0){//nghĩa là table chưa có IP này.
		//printf("Adding new entry: %d:%x. hb: %lu\n", id, port, hb);
		//if doesnt exist, add it and log it

		Address *newAddr;
		newAddr = (Address *) malloc(1 * sizeof(Address));
		memcpy((char *)(newAddr->addr), &id, sizeof(int));
		memcpy((char *)(newAddr->addr)+sizeof(int), &port, sizeof(short));

		//Add new member to Member's list and log it
		addMemberListTable(id, port,  hb);
		log->logNodeAdd(&memberNode->addr, newAddr);
		free(newAddr);
	}

}
/**
 * Adds newMember to  to the memberList array
 */
void MP1Node::addMemberListTable(MemberListEntry newMember){
	MemberListEntry newMLE;
	memberNode->memberList.push_back(newMember);


}

/*
 * Adds a new MemberListEntry to the memberList array with the given parameters
 */
void MP1Node::addMemberListTable(int id, short port, long heartbeat){
	MemberListEntry newMLE;

	newMLE.setid(id);
	newMLE.setport(port);
	newMLE.setheartbeat(heartbeat);
	newMLE.settimestamp(memberNode->heartbeat);
	addMemberListTable(newMLE);

}

void MP1Node::writeMemberListTable(char *ptr){
	int size = memberNode->memberList.size();
	//int fakeSize = 19;
	memcpy(ptr, &size, sizeof(int));
	//printf("Writing table with %d elements to buffer.\n",size);

	int currByte = sizeof(int);
	//Copy member list to message:
	for (int i = 0; i <  memberNode->memberList.size() ;i++){

		int id = memberNode->memberList.at(i).id;
		memcpy( (ptr)+currByte, &id , sizeof (int));
		currByte = currByte + 4;

		short port = memberNode->memberList.at(i).port;
		memcpy((ptr) + currByte,&port, sizeof (short));
		currByte = currByte + 2;

		long hb = memberNode->memberList.at(i).heartbeat;
		memcpy( (ptr)+currByte, &hb, sizeof (long));
		currByte = currByte + 8;

	}



}
/**
 * FUNCTION NAME: printMemberListTable
 *
 * DESCRIPTION: 
 */
void MP1Node::printMemberListTable() {
	int i = 0;
	printf("Printing MemberListTable: %d elements. I am: ",memberNode->memberList.size()   );
	printAddress(&memberNode->addr);
	for ( i = 0; i <  memberNode->memberList.size() ; i++){
		MemberListEntry curr = memberNode->memberList.at(i);
		printf("\t%d: Address: %d:%x, Hb: %lu, Timetamp: %lu\n",
				i, curr.getid(), curr.getport() , curr.getheartbeat(), curr.gettimestamp() );


	}

}
/**
 * FUNCTION NAME: getAddress
 *
 * DESCRIPTION: 
 */
Address MP1Node::getAddress(int address, short port) {

	Address addr;
	memset(&addr, 0, sizeof(Address));
	*(int *)(&addr.addr) = address;
	*(short *)(&addr.addr[4]) = port;

	return addr;
}


/**
 * FUNCTION NAME: nodeLoopOps
 *
 * DESCRIPTION: Check if any node hasn't responded within a timeout period and then delete
 * 				the nodes
 * 				Propagate your membership list
 */
void MP1Node::nodeLoopOps() {

	/*
	 * Your code goes here
	 */
	for (int i = 0; i < memberNode->memberList.size();i++){
		int id = memberNode->memberList.at(i).id;
		short port = memberNode->memberList.at(i).port;
		long lastHb = memberNode->memberList.at(i).heartbeat;
		long timestamp = memberNode->memberList.at(i).timestamp;

		long diff = memberNode->heartbeat - timestamp;
		//printf("checking life.. %d:%x .. diff is: %lu \n" , id, port, diff);

		if (lastHb != -1 && diff > TREMOVE + TFAIL){
			printf("Removing %d:%x .. diff was: %lu " , id, port, diff);
			Address currAddr = getAddress(id,port);
			memberNode->memberList.at(i).setheartbeat(-1);
			log->logNodeRemove(&memberNode->addr, &currAddr);
		}



	}



	//send a heartbeat and propagate memberlist pigbacked

	MessageHdr *msg;
	size_t msgsize = sizeof(MessageHdr) + sizeof(memberNode->addr)																		+ 1 + sizeof(long) + sizeof(int) +  memberNode->memberList.size()*14 ;
	msg = (MessageHdr *) malloc(msgsize * sizeof(char));

	// 4 bytes: Type:
	msg->msgType = DUMMYLASTMSGTYPE;

	// 6 bytes: address
	memcpy((char *)(msg+1), &memberNode->addr.addr, sizeof(memberNode->addr.addr));

	//8 bytes: heartbeat + 1 byte padding
	memcpy((char *)(msg+1) + 1 + sizeof(memberNode->addr.addr), &memberNode->heartbeat, sizeof(long));

	int currentOffset = 4+1+ sizeof(memberNode->addr.addr) + sizeof(long);
	char *ptr = ((char*) msg + currentOffset);
	writeMemberListTable(ptr);



	//broadcast
	for (int i = 0; i < memberNode->memberList.size();i++){
		Address *destinationAddr;
		destinationAddr = (Address *) malloc(1 * sizeof(Address));
		int id = memberNode->memberList.at(i).id;
		short port = memberNode->memberList.at(i).port;

		memcpy((char *)(destinationAddr->addr), &id, sizeof(int));
		memcpy((char *)(destinationAddr->addr+sizeof(int)), &port, sizeof(short));


		emulNet->ENsend(&memberNode->addr, destinationAddr, (char *)msg, msgsize);
		free (destinationAddr);

	}
	free (msg);
    return;
}

/**
 * FUNCTION NAME: isNullAddress
 *
 * DESCRIPTION: Function checks if the address is NULL
 */
int MP1Node::isNullAddress(Address *addr) {
	return (memcmp(addr->addr, NULLADDR, 6) == 0 ? 1 : 0);
}

/**
 * FUNCTION NAME: getJoinAddress
 *
 * DESCRIPTION: Returns the Address of the coordinator
 */
Address MP1Node::getJoinAddress() {
    Address joinaddr;

    memset(&joinaddr, 0, sizeof(Address));/*void init() {
							memset(&addr, 0, sizeof(addr));
							}*/
    *(int *)(&joinaddr.addr) = 1;
    *(short *)(&joinaddr.addr[4]) = 0;

    return joinaddr;
}

/**
 * FUNCTION NAME: initMemberListTable
 *
 * DESCRIPTION: Initialize the membership list
 */
void MP1Node::initMemberListTable(Member *memberNode) {
	memberNode->memberList.clear();
}

/**
 * FUNCTION NAME: printAddress
 *
 * DESCRIPTION: Print the Address
 */
void MP1Node::printAddress(Address *addr)
{
    printf("%d.%d.%d.%d:%d \n",  addr->addr[0],addr->addr[1],addr->addr[2],
                                                       addr->addr[3], *(short*)&addr->addr[4]) ;// ko có tác dụng   
}
