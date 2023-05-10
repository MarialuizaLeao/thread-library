# PAGINADOR DE MEMÓRIA - RELATÓRIO

1. Termo de compromisso

Os membros do grupo afirmam que todo o código desenvolvido para este
trabalho é de autoria própria.  Exceto pelo material listado no item
3 deste relatório, os membros do grupo afirmam não ter copiado
material da Internet nem ter obtido código de terceiros.

2. Membros do grupo e alocação de esforço

Preencha as linhas abaixo com o nome e o e-mail dos integrantes do
grupo.  Substitua marcadores `XX` pela contribuição de cada membro
do grupo no desenvolvimento do trabalho (os valores devem somar
100%).

  * Maria Luiza Leao <maria_luiza1598@hotmail.com> 50%
  * Isabella Vignoli Gonçalves <isabellavignoli14@gmail.com> 50%

3. Referências bibliográficas:
    https://petbcc.ufscar.br/time/
    https://www.tutorialspoint.com/c_standard_library/time_h.htm
    https://www.ibm.com/docs/en/i/7.2?topic=ssw_ibm_i_72/apis/sigactn.html
    https://man7.org/linux/man-pages/man2/sigaction.2.html
    https://virtual.ufmg.br/20231/pluginfile.php/286104/mod_resource/content/0/06-Sincronizacao.pdf
    https://www.geeksforgeeks.org/use-posix-semaphores-c/
    https://greenteapress.com/thinkos/html/thinkos013.html
    https://man7.org/linux/man-pages/man3/makecontext.3.html
    https://man7.org/linux/man-pages/man3/getcontext.3.html
    https://man7.org/linux/man-pages/man3/swapcontext.3.html
    https://man7.org/linux/man-pages/man3/setcontext.3.html


4. Estruturas de dados

  1. Descreva e justifique as estruturas de dados utilizadas para
    As estruturas de dados utilizadas na implementação da biblioteca de threads de espaço do usuário apresentada são:

Para gerenciar as threads de espaço do usuário, o código utiliza a estrutura dccthread_t que contém informações sobre a thread como seu nome, contexto e estado de espera. Além disso, o código também usa a estrutura bin_sem_t para implementar semáforos binários e gerenciar a lista de threads que estão esperando.

A lista de threads prontas para serem executadas é armazenada em uma lista duplamente ligada readyThreadList, implementada pela biblioteca dlist.h.

Quando a função dccthread_init é chamada, ela inicializa a lista readyThreadList e cria a thread principal chamada main. Em seguida, ela configura o temporizador e o sinal de preempção para interromper a execução da thread atual e transferir o controle para outra thread. A função entra em um loop em que as threads prontas são executadas e as threads bloqueadas esperando são adicionadas à lista binSem->sleepingThreadList.

A função dccthread_create é responsável por criar uma nova thread, alocando memória para a estrutura dccthread_t e para o contexto da nova thread. O contexto é configurado para chamar a função func com o parâmetro param e, em seguida, transferir o controle de volta para a thread principal quando a função func retornar.

As funções dccthread_yield, dccthread_self, dccthread_name, dccthread_exit, dccthread_wait, dccthread_sleep e dccthread_wake_up manipulam os atributos da estrutura dccthread_t para executar as ações correspondentes.

  2. Descreva o mecanismo utilizado para sincronizar chamadas de
     dccthread_yield e disparos do temporizador (parte 4).

Na função dccthread_init(), a lista readyThreadList é criada para armazenar as threads prontas para executar, o semáforo binário binSem é inicializado com o valor 0 e uma lista vazia para armazenar as threads bloqueadas esperando pelo semáforo, e a thread principal é criada com o nome "main" e com a função passada como parâmetro. Em seguida, é configurado um temporizador para interromper a execução da thread atual após um intervalo de tempo determinado. Isso é feito por meio das funções timer_create(), timer_settime() e sigaction(), que configuram o temporizador e associam a sua interrupção a um sinal definido, no caso, SIGRTMIN.

As variáveis preemptionMask e sleepMask são definidas como conjuntos de sinais. Já a máscara de sinais do processo é configurada para bloquear o sinal SIGRTMIN, que será enviado pelo temporizador, para que não seja recebido enquanto uma thread está em execução. A máscara de sinais do processo também é configurada para bloquear o sinal SIGRTMAX, que será usado para bloquear uma thread esperando pelo semáforo.

A partir daí, um loop é iniciado para executar as threads. Primeiro, o sinal SIGRTMAX é desbloqueado para permitir que threads bloqueadas esperando pelo semáforo possam receber esse sinal. Em seguida, a thread é bloqueada novamente com a máscara de sinais do processo configurada para bloquear SIGRTMAX. A próxima thread a ser executada é obtida a partir da lista readyThreadList. Se a thread estiver esperando pelo semáforo, ela é colocada no final da lista e o loop continua com a próxima thread. Caso contrário, a thread é executada com a função swapcontext() e removida da lista readyThreadList. A função swapcontext() troca o contexto da thread atual para o contexto da próxima thread a ser executada, permitindo que a próxima thread execute de onde parou e retomando a execução da thread atual posteriormente.

A função dccthread_create() cria uma nova thread com o nome e a função passados como parâmetros. A nova thread é adicionada à lista readyThreadList e seu contexto é inicializado com a função passada como parâmetro e com a pilha alocada dinamicamente. É importante destacar que, para evitar problemas de vazamento de memória, a pilha é desalocada ao final da execução da thread.

A função dccthread_yield() interrompe a execução da thread atual e adiciona-a ao final da lista readyThreadList, permitindo que a próxima thread seja executada. A thread atual só será executada novamente quando sua vez chegar novamente na lista readyThreadList.

A função dccthread_sleep() bloqueia a execução da thread atual e adiciona-a à lista sleepingThreadList do semáforo binário. A thread atual só será executada novamente quando o semáforo for sinalizado por outra thread, removendo-a da lista sleepingThreadList.




