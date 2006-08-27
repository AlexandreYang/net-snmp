config_require(hardware/cpu/cpu)

#if defined(linux)
config_require(hardware/cpu/cpu_linux)

#elif (defined(freebsd2) || defined(freebsd3) || defined(freebsd4)  || defined(freebsd5)|| defined(freebsd6))
config_require(hardware/cpu/cpu_nlist)

#elif (defined(netbsd) || defined(netbsd1) || defined(netbsdelf) || defined(netbsdelf2)|| defined(netbsdelf3) || defined(openbsd2)|| defined(openbsd3))
config_require(hardware/cpu/cpu_sysctl)

#elif (defined(aix4) || defined(aix5))
config_require(hardware/cpu/cpu_perfstat)

#else
config_require(hardware/cpu/cpu_null)
#endif
