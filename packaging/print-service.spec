%global DATADIR /opt

Name:       print-service
Summary:    print service library
Version:    1.2.4
Release:    2
Group:      System/Libraries
License:    Flora Software License
Source0:    %{name}-%{version}.tar.gz
BuildRequires: cmake
#BuildRequires: pkgconfig(avahi-core)
#BuildRequires: pkgconfig(avahi-client)
BuildRequires: pkgconfig(dlog)
BuildRequires: pkgconfig(eina)
BuildRequires: pkgconfig(ecore)
BuildRequires: pkgconfig(vconf)
#BuildRequires: pkgconfig(sqlite3)
BuildRequires: binutils-devel
BuildRequires: cups-devel
BuildRequires: libxml2-devel
BuildRequires: capi-appfw-application-devel
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
#%define CMAKE_TMP_DIR cmake_tmp

#mkdir -p %{CMAKE_TMP_DIR}
#cd %{CMAKE_TMP_DIR}
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

mkdir -p /etc/udev/rules.d
if ! [ -L /etc/udev/rules.d/91-print-service.rules ]; then
        ln -s /usr/share/print-service/udev-rules/91-print-service.rules /etc/udev/rules.d/91-print-service.rules
fi
#vconftool -i set -t int memory/Device/usbhost/status 0

%post -n print-driver-data
mkdir -p /opt/dbspace
if [ -f /opt/dbspace/.ppd.db ]
then
    rm -f /opt/dbspace/.ppd*
fi

#sqlite3 /opt/dbspace/.ppd.db < /opt/etc/ppd_db.sql
#sqlite3 /opt/dbspace/.ppd.db < /opt/etc/ppd_db_data.sql

#rm -f /opt/etc/ppd_db.sql
#rm -f /opt/etc/ppd_db_data.sql

mkdir -p /usr/share/cups/model/samsung
ln -sf /opt/etc/cups/ppd/samsung/cts_files /usr/share/cups/model/samsung/cms

chown -R 5000:5000 /opt/etc/cups/ppd

%postun -n print-driver-data
rm /usr/share/cups/model/samsung/cms
rmdir /usr/share/cups/model/samsung

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
#%doc LICENSE
/usr/share/license/%{name}
%{_bindir}/getdrv
%{_libdir}/*.so*
%exclude %{_libdir}/libopmap.so*
%dir /usr/share/print-service/udev-rules
/usr/share/print-service/udev-rules/91-print-service.rules

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
%attr(-,app,app) %{DATADIR}/etc/cups/ppd/epson.list
%attr(-,app,app) %{DATADIR}/etc/cups/ppd/samsung.list

%files -n print-driver-data
%manifest printer-driver-data.manifest
%defattr(-,root,root,-)
/usr/share/license/print-driver-data
#%{DATADIR}/etc/ppd_db.sql
#%{DATADIR}/etc/ppd_db_data.sql
%attr(-,app,app) %dir %{DATADIR}/etc/cups/ppd
%attr(-,app,app) %{DATADIR}/etc/cups/ppd/*
%exclude %{DATADIR}/etc/cups/ppd/hp.list
%exclude %{DATADIR}/etc/cups/ppd/epson.list
%exclude %{DATADIR}/etc/cups/ppd/samsung.list
#%attr(-,app,app) %{DATADIR}/etc/cups/ppd/Generic/*
#%attr(-,app,app) %{DATADIR}/etc/cups/ppd/hp/*
#%attr(-,app,app) %{DATADIR}/etc/cups/ppd/samsung/*.ppd.gz
#%attr(-,app,app) %{DATADIR}/etc/cups/ppd/epson/*
#%{DATADIR}/etc/cups/ppd/samsung/*.xml
#%{DATADIR}/etc/cups/ppd/samsung/cts_files/*
%changelog
