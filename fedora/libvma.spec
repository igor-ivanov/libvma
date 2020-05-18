%{!?configure_options: %global configure_options %{nil}}
%{!?use_rel: %global use_rel 1}

%{!?make_build: %global make_build %{__make} %{?_smp_mflags} %{?mflags} V=1}
%{!?_pkgdocdir: %global _pkgdocdir %{_docdir}/%{name}-%{version}}

Name: libvma
Version: 9.0.2
Release: %{use_rel}%{?dist}
Summary: A library for boosting TCP and UDP traffic (over RDMA hardware)

License: GPLv2 or BSD
Url: https://github.com/Mellanox/libvma
Source0: https://github.com/Mellanox/libvma/archive/%{version}/%{name}-%{version}.tar.gz

# libvma currently supports only the following architectures
ExclusiveArch: x86_64 ppc64le ppc64 aarch64

BuildRequires: pkgconfig
BuildRequires: automake
BuildRequires: autoconf
BuildRequires: libtool
BuildRequires: gcc-c++
BuildRequires: libibverbs-devel
BuildRequires: librdmacm-devel
BuildRequires: libnl3-devel
BuildRequires: systemd-rpm-macros

%description
libvma is a LD_PRELOAD-able library that boosts performance of TCP and
UDP traffic. It allows application written over standard socket API to
handle fast path data traffic from user space over Ethernet and/or
Infiniband with full network stack bypass and get better throughput,
latency and packets/sec rate.

No application binary change is required for that.
libvma is supported by RDMA capable devices that support "verbs"
IBV_QPT_RAW_PACKET QP for Ethernet and/or IBV_QPT_UD QP for IPoIB.

This package includes headers for building programs with libvma's interface
directly, as opposed to loading it dynamically with LD_PRELOAD.

%package devel
Summary: Header files and link required to develop with Libvma
Requires: %{name}%{?_isa} = %{version}-%{release}

%description devel
This package includes headers for building programs with libvma's
interfaces.

%package utils
Summary: Libvma utilities
Requires: %{name}%{?_isa} = %{version}-%{release}

%description utils
This package contains the tool vma_stats for collecting and
analyzing Libvma statistic.

%prep
%setup -q

%build
export revision=%{use_rel}
if [ ! -e configure ] && [ -e autogen.sh ]; then
    VMA_RELEASE=%{use_rel} ./autogen.sh
fi

%configure --docdir=%{_pkgdocdir} \
           %{?configure_options}
%{make_build}

%install
%{make_build} DESTDIR=${RPM_BUILD_ROOT} install

find $RPM_BUILD_ROOT%{_libdir} -name '*.la' -delete

install -m 644 src/vma/vma_extra.h $RPM_BUILD_ROOT/%{_includedir}/mellanox/vma_extra.h
install -m 644 src/vma/util/libvma.conf $RPM_BUILD_ROOT/%{_sysconfdir}/
install -s -m 755 src/stats/vma_stats $RPM_BUILD_ROOT/%{_bindir}/vma_stats
install -s -m 755 tools/daemon/vmad $RPM_BUILD_ROOT/%{_sbindir}/vmad
install -D -m 644 contrib/scripts/vma.service $RPM_BUILD_ROOT/%{_unitdir}/vma.service
install -m 755 contrib/scripts/vma.init $RPM_BUILD_ROOT/%{_sbindir}/vma

%post
%{?ldconfig}
%systemd_post vma.service

%preun
%systemd_preun vma.service

%postun
%{?ldconfig}
%systemd_postun_with_restart vma.service

%files
%{_libdir}/%{name}.so*
%doc %{_pkgdocdir}/README.txt
%doc %{_pkgdocdir}/journal.txt
%doc %{_pkgdocdir}/VMA_VERSION
%config(noreplace) %{_sysconfdir}/libvma.conf
%config(noreplace) %{_sysconfdir}/security/limits.d/30-libvma-limits.conf
%{_sbindir}/vmad
%{_prefix}/lib/systemd/system/vma.service
%{_sbindir}/vma
%license COPYING

%files devel
%{_includedir}/mellanox/vma_extra.h

%files utils
%{_bindir}/vma_stats

%changelog
* Fri Apr 17 2020 Igor Ivanov <igor.ivanov.va@gmail.com> 9.0.2-1
- Align with Fedora guidelines

* Thu Feb 7 2019 Igor Ivanov <igor.ivanov.va@gmail.com> 8.8.2-1
- Improve package update processing

* Tue Dec 19 2017 Igor Ivanov <igor.ivanov.va@gmail.com> 8.5.1-1
- Add systemd support

* Tue May 9 2017 Ophir Munk <ophirmu@mellanox.com> 8.3.4-1
- Add libvma-debug.so installation

* Mon Nov 28 2016 Igor Ivanov <igor.ivanov.va@gmail.com> 8.2.2-1
- Add daemon

* Mon Jan  4 2016 Avner BenHanoch <avnerb@mellanox.com> 7.0.12-1
- Initial Packaging
