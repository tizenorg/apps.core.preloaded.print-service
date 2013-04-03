%global DATADIR /opt

Name:       print-service
Summary:    print service library
Version:    1.2.8
Release:    1
Group:      System/Libraries
License:    Flora Software License
Source0:    %{name}-%{version}.tar.gz
BuildRequires: cmake
BuildRequires: pkgconfig(dlog)
BuildRequires: pkgconfig(eina)
BuildRequires: pkgconfig(ecore)
BuildRequires: pkgconfig(vconf)
BuildRequires: glib2-devel
BuildRequires: binutils-devel
BuildRequires: cups-devel
BuildRequires: libxml2-devel
BuildRequires: capi-appfw-application-devel
Requires: glib2
Requires: cups
Requires(post):  /sbin/ldconfig
Requires(postun):  /sbin/ldconfig
BuildRoot:  %{_tmppath}/%{name}-%{version}-build

%description
print-service library

%package devel
Summary:    print library - development file
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
print library - development file

%package -n print-driver-data
Summary:    printer data - ppd, cts, data files
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description -n print-driver-data
printer data - ppd, cts, data files

%package tests
Summary:    testing utilities
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description tests
Set of utilities for testing different parts of library

%prep
%setup -q

%build
cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix} -DENABLE_OM_TESTS=On

%install
rm -rf %{buildroot}
%make_install
mkdir -p %{buildroot}/usr/share/license
cp %{_builddir}/%{buildsubdir}/LICENSE.Flora %{buildroot}/usr/share/license/%{name}
cp %{_builddir}/%{buildsubdir}/LICENSE.Flora %{buildroot}/usr/share/license/print-driver-data

%clean
rm -rf %{buildroot}

%post
/sbin/ldconfig

if ! [ -d /opt/etc/cups/ppd/hp ]
then
	mkdir -p /opt/etc/cups/ppd/hp
fi
if ! [ -d /opt/etc/cups/ppd/epson ]
then
	mkdir -p /opt/etc/cups/ppd/epson
fi
if ! [ -d /opt/etc/cups/ppd/samsung ]
then
	mkdir -p /opt/etc/cups/ppd/samsung
fi
chown -R 5000:5000 /opt/etc/cups/ppd

if [ -f /usr/lib/rpm-plugins/msm.so ]
then
	chsmack -a mobileprint /opt/etc/cups/ppd/
	chsmack -a mobileprint /opt/etc/cups/ppd/hp
	chsmack -a mobileprint /opt/etc/cups/ppd/epson
	chsmack -a mobileprint /opt/etc/cups/ppd/samsung
	chsmack -t /opt/etc/cups/ppd
	chsmack -t /opt/etc/cups/ppd/hp
	chsmack -t /opt/etc/cups/ppd/epson
	chsmack -t /opt/etc/cups/ppd/samsung
fi

%post -n print-driver-data
mkdir -p /usr/share/cups/model/samsung
ln -sf /usr/share/cups/ppd/samsung/cms /usr/share/cups/model/samsung/cms

%postun -n print-driver-data
if [ -e /usr/share/cups/model/samsung/cms ]
then
	rm /usr/share/cups/model/samsung/cms
fi
if [ -f /opt/etc/cups/ppd/hp/hp.drv ]
then
	rm /opt/etc/cups/ppd/hp/hp.drv
fi
if [ -f /opt/etc/cups/ppd/samsung/samsung.drv ]
then
	rm /opt/etc/cups/ppd/samsung/samsung.drv
fi
if [ -f /opt/etc/cups/ppd/epson/epson.drv ]
then
	rm /opt/etc/cups/ppd/epson/epson.drv
fi

%postun
/sbin/ldconfig

%post devel
chmod 644 /usr/lib/pkgconfig/print-service.pc
chmod 644 /usr/include/print-service/pt_api.h

%files
%manifest print-service.manifest
%defattr(-,root,root,-)
%attr(0755,root,root) %{_bindir}/getppd
/usr/share/license/%{name}
%{_libdir}/*.so*
%exclude %{_libdir}/libopmap.so*

%files devel
%defattr(-,root,root,-)
%{_includedir}/print-service/*.h
%{_libdir}/pkgconfig/*

%files -n print-driver-data
%manifest print-driver-data.manifest
%defattr(-,root,root,-)
/usr/share/license/print-driver-data
%dir /usr/share/cups/ppd/
/usr/share/cups/ppd/*
%exclude %{DATADIR}/etc/cups/ppd/hp_product.list
%exclude %{DATADIR}/etc/cups/ppd/hp.list
%exclude %{DATADIR}/etc/cups/ppd/epson.list
%exclude %{DATADIR}/etc/cups/ppd/samsung.list

%files -n print-service-tests
%defattr(-,root,root,-)
%{_bindir}/getppdvalue
%attr(0755,root,root) %{_bindir}/ppd_test.sh
%attr(0755,root,root) %{_bindir}/test-opmap
%attr(0755,root,root) %{_bindir}/print-test-opmap.sh
%{_libdir}/libopmap.so*
%attr(-,app,app) %{DATADIR}/etc/cups/ppd/hp.list
%attr(-,app,app) %{DATADIR}/etc/cups/ppd/hp_product.list
%attr(-,app,app) %{DATADIR}/etc/cups/ppd/epson.list
%attr(-,app,app) %{DATADIR}/etc/cups/ppd/samsung.list

%changelog

