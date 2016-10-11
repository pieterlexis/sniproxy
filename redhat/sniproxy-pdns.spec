Name: sniproxy
Version: @VERSION@
Release: 1.pdns
Summary: Transparent TLS proxy

Group: System Environment/Daemons
License: BSD
URL: https://github.com/dlundquist/sniproxy

BuildRequires: autoconf
BuildRequires: automake
BuildRequires: curl
BuildRequires: systemd
BuildRequires: libev-devel
BuildRequires: lua-devel
BuildRequires: pcre-devel
BuildRequires: perl
BuildRequires: udns-devel
# for lib-prefix.m4
BuildRequires: gettext-devel
# required for EL
BuildRequires: perl(Time::HiRes)

Requires(pre):    shadow-utils
Requires(post):   systemd
Requires(preun):  systemd
Requires(postun): systemd

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
%configure
make %{?_smp_mflags}

%install
make install DESTDIR=%{buildroot}
mkdir -p %{buildroot}%{_sysconfdir}
mkdir -p %{buildroot}%{_unitdir}
install -p -m 644 redhat/sniproxy.conf %{buildroot}%{_sysconfdir}/
install -p -m 644 redhat/sniproxy.service %{buildroot}%{_unitdir}

#%check
#make check

%pre
getent group sniproxy &>/dev/null || groupadd -r sniproxy
getent passwd sniproxy &>/dev/null || \
    /usr/sbin/useradd -r -g sniproxy -s /sbin/nologin -c sniproxy \
    -d / sniproxy

%post
%systemd_post sniproxy.service

%preun
%systemd_preun sniproxy.service

%postun
%systemd_postun sniproxy.service

%files

%config(noreplace) %{_sysconfdir}/sniproxy.conf

%{_sbindir}/sniproxy
%doc COPYING README.md AUTHORS ChangeLog
%{_unitdir}/sniproxy.service
%{_mandir}/man8/sniproxy.8*
%{_mandir}/man5/sniproxy.conf.5*
