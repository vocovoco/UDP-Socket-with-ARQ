# UDP-Socket-with-ARQ
 - Client에서 Server로 파일을 전송하는 간단한 프로그램입니다.
 - netem을 통해 loopback의 loss rate: 10%, delay: 50ms로 설정하고 실험하였습니다.
 	- 설정 생성: sudo tc qdisc add dev \<Net Interface> root netem delay \<Time>ms loss \<Persantage>%
	- 설정 확인: sudo tc qdisc show dev \<Net Interface>
	- 설정 초기화: sudo tc qdisc del dev \<Net Interface>
## Stop-N-Wait
- 전송용량: 12MB, 평균전송시간: 약 32sec, Buffer size: 50KB, Timer: 0.1sec
- Client
	1. 파일을 열고 파일 이름을 packet에 담아 전송
	2. - ack을 받은 경우, (iii)으로 이동
		- ack을 받지 못한 경우, (i)로 이동(5번 반복 후 종료: 연결되지 않음을 의미)
	3. - 파일 내용을 읽어들인 후, 읽은 크기, sequence #와 함께 packet 생성
		- 더 이상 읽을 내용이 없을 경우 (vi)으로 이동
	4. packet 전송 후 ack이 올때까지 대기
	5. - ack을 받은 경우, sequence #를 변경 후 (iii)으로 이동
  		- time-out전에 ack을 받지 못한 경우, (iv)로 이동
	6. 종료 요청 packet 전송
	7. - 종료 확인 ack을 받은 경우, socket과 file pointer를 닫고 프로그램 종료
		- 받지 못한 경우 (vi)으로 이동
		
- Server
	1. 파일 이름을 담은 packet을 받아 ack을 전송
	2. 받은 파일 이름으로 저장할 파일 생성
	3. - 현재의 sequnece #와 일치하는 파일 내용을 담은 packet을 받은 경우, 파일에 저장하고 현재 sequnece #로 ack를 보낸 후 sequnece #를 변경
		- 그렇지 않은 경우, 이전 sequnece #로 ack을 전송
	4. 종료 요청 packet을 받은 경우, 종료 확인 ack을 전송 후 sequence #를 초기화하고 file pointer를 닫음
	
## Go-Back-N
- 전송용량: 12MB, 평균전송시간: 약 12sec, Buffer size: 50KB, Timer: 0.1sec, sender window size: 4, seq# size: 16
- Client
	1. 파일을 열고 파일 이름을 packet에 담아 전송
	2. - ack을 받은 경우, (iii)으로 이동
		- ack을 받지 못한 경우, (i)로 이동(5번 반복 후 종료: 연결되지 않음을 의미)
	3. - 파일 내용을 읽어들인 후, 읽은 크기, sequence #와 함께 packet을 window size만큼 생성
		- 더 이상 읽을 내용이 없을 경우 (vi)으로 이동
	4. window size 만큼의 packet 전송 후 ack이 올때까지 대기
	5. - window안에 속한 sequence #의 ack을 받은 경우, sequence #를 증가시킨 후 (iii)으로 이동
		- window안에 속하지 않는 sequence #의 ack을 받은 경우, 받은 ack을 무시하고 대기
  		- time-out전에 window안에 속한 sequence #의 ack을 받지 못한 경우, (iv)로 이동
	6. 종료 요청 packet 전송
	7. - 종료 확인 ack을 받은 경우, socket과 file pointer를 닫고 프로그램 종료
		- 받지 못한 경우 (vi)으로 이동
		
- Server
	1. 파일 이름을 담은 packet을 받아 ack을 전송
	2. 받은 파일 이름으로 저장할 파일 생성
	3. - 현재의 sequnece #와 일치하는 파일 내용을 담은 packet을 받은 경우, 파일에 저장하고 다음 sequnece #로 ack를 보낸 후 sequnece #를 증가
		- 그렇지 않은 경우, 현재 sequnece #로 ack을 전송
	4. 종료 요청 packet을 받은 경우, 종료 확인 ack을 전송 후 sequence #를 초기화하고 file pointer를 닫음

## Selective Repeat
todo
