/*
   3APA3A simpliest proxy server
   (c) 2002-2008 by ZARAZA <3APA3A@security.nnov.ru>

   please read License Agreement

   $Id: sockmap.c,v 1.56 2009/02/25 20:04:09 vlad Exp $
*/

#include "proxy.h"

#define BUFSIZE (param->srv->bufsize?param->srv->bufsize:((param->service == S_UDPPM)?UDPBUFSIZE:TCPBUFSIZE))

int sockmap(struct clientparam * param, int timeo){
 int res=0, sent=0, received=0;
 SASIZETYPE sasize;
 struct pollfd fds[2];
 int sleeptime = 0, stop = 0;
 unsigned minsize;
 unsigned bufsize;
 FILTER_ACTION action;
 int retcode = 0;

 bufsize = BUFSIZE; 

 minsize = (param->service == S_UDPPM)? bufsize - 1 : (bufsize>>4);

 fds[0].fd = param->clisock;
 fds[1].fd = param->remsock;

#if DEBUGLEVEL > 2
(*param->srv->logfunc)(param, "Starting sockets mapping");
#endif
 if(!param->waitclient){
	if(!param->srvbuf && (!(param->srvbuf=myalloc(bufsize)) || !(param->srvbufsize = bufsize))){
		return (21);
	}
 }
 if(!param->waitserver){
	if(!param->clibuf && (!(param->clibuf=myalloc(bufsize)) || !(param->clibufsize = bufsize))){
		return (21);
	}
 }



 if(!param->nolongdatfilter){
	if(param->cliinbuf > param->clioffset){
		action = handledatfltcli(param,  &param->clibuf, &param->clibufsize, param->clioffset, &param->cliinbuf);
		if(action == HANDLED){
			return 0;
		}
		if(action != PASS) return 19;
	}
	if(param->srvinbuf > param->srvoffset){
		action = handledatfltsrv(param,  &param->srvbuf, &param->srvbufsize, param->srvoffset, &param->srvinbuf);
		if(action == HANDLED){
			return 0;
		}
		if(action != PASS) return 19;
	}
 }



 while (!stop&&!conf.timetoexit){
	sasize = sizeof(struct sockaddr_in);
	if((param->maxtrafin && param->statssrv >= param->maxtrafin) || (param->maxtrafout && param->statscli >= param->maxtrafout)){
		return (10);
	}
	if((param->srv->logdumpsrv && (param->statssrv > param->srv->logdumpsrv)) ||
		(param->srv->logdumpcli && (param->statscli > param->srv->logdumpcli)))
			(*param->srv->logfunc)(param, NULL);
	fds[0].events = fds[1].events = 0;
	if(param->srvinbuf > param->srvoffset && !param->waitclient) {
#if DEBUGLEVEL > 2
(*param->srv->logfunc)(param, "will send to client");
#endif
		fds[0].events |= POLLOUT;
	}
	if((param->srvbufsize - param->srvinbuf) > minsize && !param->waitclient) {
#if DEBUGLEVEL > 2
(*param->srv->logfunc)(param, "Will recv from server");
#endif
		fds[1].events |= POLLIN;
	}

	if(param->cliinbuf > param->clioffset && !param->waitserver) {
#if DEBUGLEVEL > 2
(*param->srv->logfunc)(param, "Will send to server");
#endif
		fds[1].events |= POLLOUT;
	}
    	if((param->clibufsize - param->cliinbuf) > minsize  && !param->waitserver &&(!param->srv->singlepacket || param->service != S_UDPPM)) {
#if DEBUGLEVEL > 2
(*param->srv->logfunc)(param, "Will recv from client");
#endif
		fds[0].events |= POLLIN;
	}
	res = poll(fds, 2, timeo*1000);
	if(res < 0){
		if(errno != EAGAIN && errno != EINTR) return 91;
		if(errno == EINTR) usleep(SLEEPTIME);
	 	continue;
	}
	if(res < 1){
		return 92;
	}
	if( (fds[0].revents & (POLLERR|POLLHUP|POLLNVAL)) && !(fds[0].revents & POLLIN)) {
		fds[0].revents = 0;
		stop = 1;
		retcode = 90;
	}
	if( (fds[1].revents & (POLLERR|POLLHUP|POLLNVAL)) && !(fds[1].revents & POLLIN)){
		fds[1].revents = 0;
		stop = 1;
		retcode = 90;
	}
	if((fds[0].revents & POLLOUT)){
#if DEBUGLEVEL > 2
(*param->srv->logfunc)(param, "send to client");
#endif
		if(param->bandlimfunc) {
			sleeptime = (*param->bandlimfunc)(param, param->srvinbuf - param->srvoffset, 0);
		}
		res = sendto(param->clisock, param->srvbuf + param->srvoffset,(!param->waitserver || ((unsigned)param->waitserver - received) > (param->srvinbuf - param->srvoffset))? param->srvinbuf - param->srvoffset : param->waitserver - received, 0, (struct sockaddr*)&param->sinc, sasize);
		if(res < 0) {
			if(errno != EAGAIN) return 96;
			continue;
		}
		param->srvoffset += res;
		received += res;
		if(param->srvoffset == param->srvinbuf) param->srvoffset = param->srvinbuf = 0;
		if(param->waitserver && param->waitserver<= received){
			return (98);
		}
		if(param->service == S_UDPPM && param->srv->singlepacket) {
			stop = 1;
		}
	}
	if((fds[1].revents & POLLOUT)){
#if DEBUGLEVEL > 2
(*param->srv->logfunc)(param, "send to server");
#endif
		if(param->bandlimfunc) {
			int sl1;

			sl1 = (*param->bandlimfunc)(param, 0, param->cliinbuf - param->clioffset);
			if(sl1 > sleeptime) sleeptime = sl1;
		}
		res = sendto(param->remsock, param->clibuf + param->clioffset, (!param->waitclient || ((unsigned)param->waitclient - sent) > (param->cliinbuf - param->clioffset))? param->cliinbuf - param->clioffset : param->waitclient - sent, 0, (struct sockaddr*)&param->sins, sasize);
		if(res < 0) {
			if(errno != EAGAIN) return 97;
			continue;
		}
		param->clioffset += res;
		sent += res;
		param->statscli += res;
		param->nwrites++;
		if(param->clioffset == param->cliinbuf) param->clioffset = param->cliinbuf = 0;
		if(param->waitclient && param->waitclient<= sent) {
			return (99);
		}
	}
	if ((fds[0].revents & POLLIN)) {
#if DEBUGLEVEL > 2
(*param->srv->logfunc)(param, "recv from client");
#endif
		res = recvfrom(param->clisock, param->clibuf + param->cliinbuf, param->clibufsize - param->cliinbuf, 0, (struct sockaddr *)&param->sinc, &sasize);
		if (res==0) {
			shutdown(param->clisock, SHUT_RDWR);
			closesocket(param->clisock);
			fds[0].fd = param->clisock = INVALID_SOCKET;
			stop = 1;
		}
		else {
			if (res < 0){
				if( errno != EAGAIN) return (94);
				continue;
			}
			param->cliinbuf += res;
			if(!param->nolongdatfilter){
				action = handledatfltcli(param,  &param->clibuf, &param->clibufsize, param->cliinbuf - res, &param->cliinbuf);
				if(action == HANDLED){
					return 0;
				}
				if(action != PASS) return 19;
			}

		}
	}
	if (!stop && (fds[1].revents & POLLIN)) {
		struct sockaddr_in sin;
#if DEBUGLEVEL > 2
(*param->srv->logfunc)(param, "recv from server");
#endif

		sasize = sizeof(sin);
		res = recvfrom(param->remsock, param->srvbuf + param->srvinbuf, param->srvbufsize - param->srvinbuf, 0, (struct sockaddr *)&sin, &sasize);
		if (res==0) {
			shutdown(param->remsock, SHUT_RDWR);
			closesocket(param->remsock);
			fds[1].fd = param->remsock = INVALID_SOCKET;
			stop = 2;
		}
		else {
			if (res < 0){
				if( errno != EAGAIN) return (93);
				continue;
			}
			param->srvinbuf += res;
			param->nreads++;
			param->statssrv += res;
			if(!param->nolongdatfilter){
				action = handledatfltsrv(param,  &param->srvbuf, &param->srvbufsize, param->srvinbuf - res, &param->srvinbuf);
				if(action == HANDLED){
					return 0;
				}
				if(action != PASS) return 19;
			}

		}
	}

	if(sleeptime > 0) {
		if(sleeptime > (timeo * 1000)){return (95);}
		usleep(sleeptime * SLEEPTIME);
		sleeptime = 0;
	}
 }
 if(conf.timetoexit) return 89;
#if DEBUGLEVEL > 2
(*param->srv->logfunc)(param, "finished with mapping");
#endif
 while(!param->waitclient && param->srvinbuf > param->srvoffset && param->clisock != INVALID_SOCKET){
#if DEBUGLEVEL > 2
(*param->srv->logfunc)(param, "flushing buffer to client");
#endif
	res = socksendto(param->clisock, &param->sinc, param->srvbuf + param->srvoffset, param->srvinbuf - param->srvoffset, conf.timeouts[STRING_S] * 1000);
	if(res > 0){
		param->srvoffset += res;
		param->statssrv += res;
		if(param->srvoffset == param->srvinbuf) param->srvoffset = param->srvinbuf = 0;
	}
	else break;
 } 
 while(!param->waitserver && param->cliinbuf > param->clioffset && param->remsock != INVALID_SOCKET){
#if DEBUGLEVEL > 2
(*param->srv->logfunc)(param, "flushing buffer to server");
#endif
	res = socksendto(param->remsock, &param->sins, param->clibuf + param->clioffset, param->cliinbuf - param->clioffset, conf.timeouts[STRING_S] * 1000);
	if(res > 0){
		param->clioffset += res;
		param->statscli += res;
		if(param->cliinbuf == param->clioffset) param->cliinbuf = param->clioffset = 0;
	}
	else break;
 } 
 return retcode;
}
