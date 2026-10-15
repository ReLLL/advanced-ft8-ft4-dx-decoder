module dx_native_modes
 use iso_c_binding
 use fst4_decode, only: fst4_decoder
 use q65_decode, only: q65_decoder
 use prog_args
 implicit none
 type(fst4_decoder), save :: fst
 type(q65_decoder), save :: qdecoder
 integer(c_short), allocatable, save :: buffer(:)
 interface
  subroutine dx_mode_result(snr,dt,freq,used,text,length) bind(C)
   import c_int,c_double,c_char
   integer(c_int),value :: snr,used,length
   real(c_double),value :: dt,freq
   character(c_char) :: text(*)
  end subroutine
 end interface
contains
 subroutine dx_native_decode(mode,period,submode,nutc,input,count,average,reset) bind(C)
  integer(c_int),value :: mode,period,submode,nutc,count,average,reset
  integer(c_short),intent(in) :: input(*)
  integer :: wspr,nfa,nfb,depth,navg,nqf(20)
  logical :: clear
  character(len=12) :: mycall,hiscall
  character(len=6) :: hisgrid
  if(.not.allocated(buffer)) allocate(buffer(21600000))
  if(count.lt.1.or.count.gt.size(buffer)) return
  buffer=0
  buffer(1:count)=input(1:count)
  data_dir='.'
  temp_dir='.'
  mycall=' '
  hiscall=' '
  hisgrid=' '
  nfa=200
  nfb=4800
  depth=2
  if(mode.eq.3) then
   clear=reset.ne.0.or.average.eq.0
   navg=0
   nqf=0
   open(17,file='red.dat',status='replace')
   open(14,file='avemsg.txt',status='replace')
   call qdecoder%decode(q_callback,buffer,1,nutc,period,submode,1500,500,depth,nfa,nfb, &
    clear,.false.,.false.,0,.true.,0.0,mycall,hiscall,hisgrid,0,0,.true.,navg,nqf)
   close(17)
   close(14)
  else
   wspr=0
   if(mode.eq.2) wspr=1
   call fst%decode(f_callback,buffer,nutc,0,nfa,nfb,1500,depth,period,768,100, &
    0.0,.false.,.true.,mycall,hiscall,wspr,.false.)
  endif
 end subroutine
 subroutine f_callback(this,nutc,sync,snr,dt,freq,decoded,nap,qual,period,fmid,w50)
  class(fst4_decoder),intent(inout) :: this
  integer,intent(in) :: nutc,snr,nap,period
  real,intent(in) :: sync,dt,freq,qual,fmid,w50
  character(len=37),intent(in) :: decoded
  call emit(snr,dt,freq,1,decoded)
 end subroutine
 subroutine q_callback(this,nutc,snr1,snr,dt,freq,decoded,idec,used,period)
  class(q65_decoder),intent(inout) :: this
  integer,intent(in) :: nutc,snr,idec,used,period
  real,intent(in) :: snr1,dt,freq
  character(len=37),intent(in) :: decoded
  call emit(snr,dt,freq,used,decoded)
 end subroutine
 subroutine emit(snr,dt,freq,used,decoded)
  integer,intent(in) :: snr,used
  real,intent(in) :: dt,freq
  character(len=37),intent(in) :: decoded
  character(c_char) :: text(37)
  integer :: i
  do i=1,37
   text(i)=decoded(i:i)
  enddo
  call dx_mode_result(snr,real(dt,c_double),real(freq,c_double),used,text,37)
 end subroutine
end module
