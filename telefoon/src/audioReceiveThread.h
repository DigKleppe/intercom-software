/*
 * audioReceiveThread.h
 *
 *  Created on: Apr 28, 2019
 *      Author: dig
 */

#ifndef AUDIORECEIVETHREAD_H_
#define AUDIORECEIVETHREAD_H_

bool setAudioReceiveTask ( streamerTask_t task, int UDPport , int SoundCardNo);
bool audioPipeLineIsStopped ( void);
bool audioReceiverIsStopped ( void);
streamerTask_t getAudioReceiveTask();

#endif /* AUDIORECEIVETHREAD_H_ */
