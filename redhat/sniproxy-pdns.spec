%if %{?rhel} < 6
exit 1
%endif

%define upstart_post() \
  if [ -x /sbin/initctl ]; then \
    /sbin/initctl start %1 \
  fi\
%{nil}

%define upstart_postun() \
  if [ -x /sbin/initctl ] && /sbin/initctl status %1 2>/dev/null | grep -q 'running' ; then \
    /sbin/initctl stop %1 >/dev/null 2>&1 \
    [ -f /etc/init/%1.conf ] && { echo -n "Re-"; /sbin/initctl start %1; }; \
  fi \
%{nil}

Name: sniproxy
Version: @VERSION@
Release: 1pdns%{?dist}
Summary: Transparent TLS proxy

Group: System Environment/Daemons
License: BSD
URL: https://github.com/dlundquist/sniproxy

BuildRequires: autoconf
BuildRequires: automake
BuildRequires: curl
BuildRequires: libev-devel
BuildRequires: luajit-devel
BuildRequires: pcre-devel
BuildRequires: perl
BuildRequires: udns-devel
# for lib-prefix.m4
BuildRequires: gettext-devel
# required for EL
BuildRequires: perl(Time::HiRes)

Requires(pre):    shadow-utils
%if %{?rhel} >= 7
BuildRequires: systemd
Requires(post): systemd-units
Requires(preun): systemd-units
Requires(postun): systemd-units
%else
Requires: util-linux-ng
%endif

%description
SNIproxy proxies incoming HTTP and TLS connections based on the host name
contained in the initial request. This enables HTTPS name based virtual
hosting to separate back-end servers without the installing the private
key on the proxy machine.
This modified version supports loading a Lua script to take decisions on
whether to allow or deny connections.


%build
touch config.rpath
autoreconf -fvi
CFLAGS="${CFLAGS:-%optflags} -I/usr/include/libev" ; export CFLAGS ;
%configure --with-luajit
make %{?_smp_mflags}

%install
make install DESTDIR=%{buildroot}
mkdir -p %{buildroot}%{_sysconfdir}
install -p -m 644 redhat/sniproxy.conf %{buildroot}%{_sysconfdir}/
%if 0%{?rhel} >= 7
mkdir -p %{buildroot}%{_unitdir}
install -p -m 644 redhat/sniproxy.service %{buildroot}%{_unitdir}
%else
mkdir -p %{buildroot}%{_sysconfdir}/init
install -p -m 644 redhat/sniproxy-upstart.conf %{buildroot}%{_sysconfdir}/init/sniproxy.conf
%endif

#%check
#make check

%pre
getent group sniproxy &>/dev/null || groupadd -r sniproxy
getent passwd sniproxy &>/dev/null || \
    /usr/sbin/useradd -r -g sniproxy -s /sbin/nologin -c sniproxy \
    -d / sniproxy

%post
%if 0%{?rhel} >= 7
%systemd_post sniproxy.service
%else
%upstart_post sniproxy
%endif

%if 0%{?rhel} >= 7
%preun
%systemd_preun sniproxy.service
%endif

%postun
%if 0%{?rhel} >= 7
%systemd_postun sniproxy.service
%else
%upstart_postun sniproxy
%endif

%files

%config(noreplace) %{_sysconfdir}/sniproxy.conf

%{_sbindir}/sniproxy
%doc COPYING README.md AUTHORS ChangeLog
%if 0%{?rhel} >= 7
%{_unitdir}/sniproxy.service
%else
%{_sysconfdir}/init/sniproxy.conf
%endif
%{_mandir}/man8/sniproxy.8*
%{_mandir}/man5/sniproxy.conf.5*
