Name:       print-service
Summary:    Print service library
Version:    1.2.9
Release:    0
Group:      System/Libraries
License:    Apache-2.0 and GPL-2.0 and MIT
Source0:    %{name}-%{version}.tar.gz
Source1001: print-service.manifest
Source1002: print-driver-data.manifest
Source1003: print-service-tests.manifest
BuildRequires:    cmake
BuildRequires:    pkgconfig(dlog)
BuildRequires:    pkgconfig(eina)
BuildRequires:    pkgconfig(ecore)
BuildRequires:    pkgconfig(vconf)
BuildRequires:    pkgconfig(libtzplatform-config)
BuildRequires:    glib2-devel
BuildRequires:    binutils-devel
BuildRequires:    cups-devel
BuildRequires:    libxml2-devel
BuildRequires:    capi-appfw-application-devel
Requires:         tizen-platform-config-tools
Requires:         cups
Requires(post):   /sbin/ldconfig
Requires(postun): /sbin/ldconfig

BuildRoot:  %{_tmppath}/%{name}-%{version}-build

%description
Print-service library

%package devel
Summary:    Print library - development file
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
Print library - development file

%package -n print-driver-data
Summary:    Printer data - ppd, cts, data files
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description -n print-driver-data
Printer data - ppd, cts, data files

%package tests
Summary:    Testing utilities
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description tests
Set of utilities for testing different parts of library

%prep
%setup -q

%build
cp %{SOURCE1001} .
cp %{SOURCE1002} .
cp %{SOURCE1003} .
%cmake . -DENABLE_OM_TESTS=On -DCMAKE_ETC=%{TZ_SYS_ETC}

%install
rm -rf %{buildroot}
%make_install
mkdir -p %{buildroot}/usr/share/license
cp %{_builddir}/%{buildsubdir}/LICENSE.APLv2 %{buildroot}/usr/share/license/%{name}
cp %{_builddir}/%{buildsubdir}/LICENSE.GPLv2 %{buildroot}/usr/share/license/print-driver-data
cat %{_builddir}/%{buildsubdir}/LICENSE.MIT >> %{buildroot}/usr/share/license/print-driver-data

%clean
rm -rf %{buildroot}

%post
/sbin/ldconfig

if ! [ -d %{TZ_SYS_ETC}/cups/ppd/hp ]
then
	mkdir -p %{TZ_SYS_ETC}/cups/ppd/hp
fi
if ! [ -d %{TZ_SYS_ETC}/cups/ppd/epson ]
then
	mkdir -p %{TZ_SYS_ETC}/cups/ppd/epson
fi
if ! [ -d %{TZ_SYS_ETC}/cups/ppd/samsung ]
then
	mkdir -p %{TZ_SYS_ETC}/cups/ppd/samsung
fi
chown -R :%{TZ_SYS_USER_GROUP} %{TZ_SYS_ETC}/cups/ppd

%post -n print-driver-data
mkdir -p /usr/share/cups/model/samsung
ln -sf /usr/share/cups/ppd/samsung/cms /usr/share/cups/model/samsung/cms

%postun -n print-driver-data
if [ -e /usr/share/cups/model/samsung/cms ]
then
	rm /usr/share/cups/model/samsung/cms
fi
if [ -f %{TZ_SYS_ETC}/cups/ppd/hp/hp.drv ]
then
	rm %{TZ_SYS_ETC}/cups/ppd/hp/hp.drv
fi
if [ -f %{TZ_SYS_ETC}/cups/ppd/samsung/samsung.drv ]
then
	rm %{TZ_SYS_ETC}/cups/ppd/samsung/samsung.drv
fi
if [ -f %{TZ_SYS_ETC}/cups/ppd/epson/epson.drv ]
then
	rm %{TZ_SYS_ETC}/cups/ppd/epson/epson.drv
fi

%postun
/sbin/ldconfig


%files
%manifest print-service.manifest
%defattr(-,root,root,-)
%attr(0755,root,root) %{_bindir}/getppd
/usr/share/license/%{name}
%{_libdir}/*.so*
%exclude %{_libdir}/libopmap.so*

%files devel
%defattr(644,root,root,-)
%{_includedir}/print-service/*.h
%{_libdir}/pkgconfig/*

%files -n print-driver-data
%manifest print-driver-data.manifest
%defattr(-,root,root,-)
/usr/share/license/print-driver-data
%dir /usr/share/cups/ppd/
/usr/share/cups/ppd/*
%exclude %{TZ_SYS_ETC}/cups/ppd/hp_product.list
%exclude %{TZ_SYS_ETC}/cups/ppd/hp.list
%exclude %{TZ_SYS_ETC}/cups/ppd/epson.list
%exclude %{TZ_SYS_ETC}/cups/ppd/samsung.list

%files -n print-service-tests
%manifest print-service-tests.manifest
%defattr(-,root,root,-)
%{_bindir}/getppdvalue
%attr(0755,root,root) %{_bindir}/ppd_test.sh
%attr(0755,root,root) %{_bindir}/test-opmap
%attr(0755,root,root) %{_bindir}/print-test-opmap.sh
%{_libdir}/libopmap.so*
%attr(-,-,%{TZ_SYS_USER_GROUP}) %{TZ_SYS_ETC}/cups/ppd/hp.list
%attr(-,-,%{TZ_SYS_USER_GROUP}) %{TZ_SYS_ETC}/cups/ppd/hp_product.list
%attr(-,-,%{TZ_SYS_USER_GROUP}) %{TZ_SYS_ETC}/cups/ppd/epson.list
%attr(-,-,%{TZ_SYS_USER_GROUP}) %{TZ_SYS_ETC}/cups/ppd/samsung.list

%changelog

