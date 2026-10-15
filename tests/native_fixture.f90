! Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
! Test waveforms use the pinned upstream encoders, never recorded receptions.
subroutine dx_native_fixture(mode,period,submode,input,count) bind(C)
 use iso_c_binding
 implicit none
 integer(c_int),value :: mode,period,submode,count
 integer(c_short),intent(out) :: input(*)
 integer :: nsps,i,j,k,nwave,wspr,i3,n3,tones(160)
 integer(c_signed_char) :: bits(101)
 real,allocatable :: wave(:)
 complex,allocatable :: cwave(:)
 real(c_double) :: phase,step,pi,baud
 character(len=37) :: msg,sent
 input(1:count)=0
 pi=acos(-1.d0)
 if(mode.eq.3) then
  msg='CQ K1ABC FN42'
  nsps=0
  if(period.eq.15) nsps=1800
  if(period.eq.30) nsps=3600
  if(period.eq.60) nsps=7200
  if(period.eq.120) nsps=16000
  if(period.eq.300) nsps=41472
  if(nsps.eq.0) return
  call genq65(msg,0,sent,tones(1:85),i3,n3)
  k=12000
  if(period.le.30) k=6000
  phase=0.d0
  baud=12000.d0/nsps*2**submode
  do j=1,85
   step=2*pi*(1500.d0+tones(j)*baud)/12000.d0
   do i=1,nsps
    k=k+1
    phase=modulo(phase+step,2*pi)
    if(k.le.count) input(k)=int(10000*sin(phase),c_short)
   enddo
  enddo
 else
  nsps=0
  if(period.eq.15) nsps=720
  if(period.eq.30) nsps=1680
  if(period.eq.60) nsps=3888
  if(period.eq.120) nsps=8200
  if(period.eq.300) nsps=21504
  if(period.eq.900) nsps=66560
  if(period.eq.1800) nsps=134400
  if(nsps.eq.0) return
  msg='CQ K1ABC FN42'
  wspr=0
  if(mode.eq.2) then
   wspr=1
   msg='K1ABC FN42 37'
  endif
  call genfst4(msg,0,sent,bits,tones,wspr)
  nwave=max(count,162*nsps)
  allocate(wave(nwave),cwave(nwave))
  call gen_fst4wave(tones,160,nsps,nwave,12000.,1,1500.+18000./nsps,0,cwave,wave)
  k=12000
  if(period.eq.15) k=6000
  do i=1,min(count-k,size(wave))
   input(i+k)=int(10000*wave(i),c_short)
  enddo
 endif
end subroutine
