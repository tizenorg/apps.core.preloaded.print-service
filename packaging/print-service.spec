%global DATADIR /opt

Name:       print-service
Summary:    print service library
Version:    1.2.4
Release:    14
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
# 1. libraries
#    chown root:root /usr/lib/libcups.so
#    chown root:root /usr/lib/libcupsmime.so
#    chown root:root /usr/lib/libcupsppdc.so
#    chown root:root /usr/lib/libcupsimage.so
#    chown root:root /usr/lib/libcupsdriver.so
# 2. executables
# 3. configurations
    #chown root:root /usr/lib/pkgconfig/cups.pc
#    chown root:root /usr/include/cups/cups.h
#    chown root:root /usr/include/cups/dir.h
#    chown root:root /usr/include/cups/driver.h
#    chown root:root /usr/include/cups/file.h
#    chown root:root /usr/include/cups/http.h
#    chown root:root /usr/include/cups/adminutil.h
#    chown root:root /usr/include/cups/array.h
#    chown root:root /usr/include/cups/backend.h
#    chown root:root /usr/include/cups/mime.h
#    chown root:root /usr/include/cups/ppd.h
#    chown root:root /usr/include/cups/ppdc.h
#    chown root:root /usr/include/cups/image.h
#    chown root:root /usr/include/cups/ipp.h
# Change file owner
# 1. libraries
#    chown root:root /usr/lib/libavahi-client.so.3
# 2. executables
# 3. configurations
#    chown root:root /usr/lib/pkgconfig/avahi-client.pc
#    chown root:root /usr/include/avahi-client/client.h
#    chown root:root /usr/include/avahi-client/lookup.h
#    chown root:root /usr/include/avahi-client/publish.h
# Change file owner
# 1. libraries       
#    chown root:root /usr/lib/libavahi-common.so.3
# 2. executables
# 3. configurations
    #chown root:root /usr/lib/pkgconfig/avahi-common.pc.pc
#    chown root:root /usr/include/avahi-common/address.h
#    chown root:root /usr/include/avahi-common/alternative.h
#    chown root:root /usr/include/avahi-common/cdecl.h
#    chown root:root /usr/include/avahi-common/defs.h
#    chown root:root /usr/include/avahi-common/domain.h
#    chown root:root /usr/include/avahi-common/error.h
#    chown root:root /usr/include/avahi-common/gccmacro.h
#    chown root:root /usr/include/avahi-common/llist.h
#    chown root:root /usr/include/avahi-common/malloc.h
#    chown root:root /usr/include/avahi-common/rlist.h
#    chown root:root /usr/include/avahi-common/simple-watch.h
#    chown root:root /usr/include/avahi-common/strlst.h
#    chown root:root /usr/include/avahi-common/thread-watch.h
#    chown root:root /usr/include/avahi-common/timeval.h
#    chown root:root /usr/include/avahi-common/watch.h
# 1. libraries    
#    chown root:root /usr/lib/libavahi-core.so.5
# 2. executables
# 3. configurations
#    chown root:root /usr/lib/pkgconfig/avahi-core.pc
# Change file permissions
# 3. configurations
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
%{_bindir}/getppdvalue
%attr(0755,root,root) %{_bindir}/ppd_test.sh
%{_includedir}/print-service/*.h
%{_libdir}/pkgconfig/*

# OptionMapping testing
%attr(0755,root,root) %{_bindir}/test-opmap
%attr(0755,root,root) %{_bindir}/print-test-opmap.sh
%{_libdir}/libopmap.so*
%attr(-,app,app) %{DATADIR}/etc/cups/ppd/hp.list
%attr(-,app,app) %{DATADIR}/etc/cups/ppd/hp_product.list
%attr(-,app,app) %{DATADIR}/etc/cups/ppd/epson.list
%attr(-,app,app) %{DATADIR}/etc/cups/ppd/samsung.list

%files -n print-driver-data
%manifest print-driver-data.manifest
%defattr(-,root,root,-)
/usr/share/license/print-driver-data
%dir /usr/share/cups/ppd/
/usr/share/cups/ppd/*
#/usr/share/cups/ppd/samsung/cms/*
%exclude %{DATADIR}/etc/cups/ppd/hp_product.list
%exclude %{DATADIR}/etc/cups/ppd/hp.list
%exclude %{DATADIR}/etc/cups/ppd/epson.list
%exclude %{DATADIR}/etc/cups/ppd/samsung.list
%changelog
