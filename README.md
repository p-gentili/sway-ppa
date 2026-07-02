# Sway 1.12 PPA for Ubuntu 26.04 (resolute)

Packaging for **Sway 1.12** and its required **wlroots 0.20**, published to
[`ppa:pgentili/sway`](https://launchpad.net/~pgentili/+archive/ubuntu/sway) for Ubuntu
26.04 (resolute), which otherwise ships only Sway 1.11 / wlroots 0.19.

Approach: rebase Ubuntu's existing 26.04 `debian/` packaging onto the new upstream
releases. Design and plan live under [`docs/superpowers/`](docs/superpowers/).

## Why two packages

Sway 1.12 requires wlroots 0.20; resolute ships wlroots 0.19. wlroots encodes its version
in the library name (`libwlroots-0.20.so` vs `libwlroots-0.19.so`), so the two coexist
without conflict — the PPA *adds* 0.20 rather than replacing the system's 0.19. Build and
upload order is always **wlroots first, then sway**, because sway build-depends on
`libwlroots-0.20-dev`.

## Installing (end users)

```bash
sudo add-apt-repository ppa:pgentili/sway
sudo apt update
sudo apt install sway
```

## Repository layout

```
wlroots/debian/   committed wlroots 0.20 packaging
sway/debian/      committed sway 1.12 packaging
build/            working area (gitignored): source trees, tarballs, .deb output
docs/superpowers/ design spec and implementation plan
```

## Prerequisites (one-time)

```bash
sudo apt install -y devscripts ubuntu-dev-tools sbuild schroot debhelper \
  quilt dput gnupg lintian
sudo sbuild-adduser "$USER"      # then log out/in (or `newgrp sbuild`)
mk-sbuild resolute               # first run opens a config editor; save, then re-run
```

A GPG key for `paolo@pgentili.com` must exist locally and be registered at
<https://launchpad.net/~pgentili/+editpgpkeys>.

### Environment note (important)

On resolute, `sbuild` defaults to **unshare** mode and ignores the `mk-sbuild` schroot.
All builds here therefore force schroot mode and run under the `sbuild` group:

```bash
sg sbuild -c 'sbuild --chroot-mode=schroot -d resolute --arch=amd64 --chroot=resolute-amd64 ...'
```

## Build + upload runbook

Everything runs from `build/`. Bump `<newver>` as needed (e.g. `0.20.1`, `1.12.1`).

### 1. wlroots

```bash
cd build
pull-lp-source wlroots resolute                 # base packaging (current archive version)
cd wlroots-<oldver>/
uscan --download-version <newver> --destdir ..  # fetch upstream tarball
cd ..
tar xf wlroots_<newver>.orig.tar.*
cp -r wlroots-<oldver>/debian wlroots-<newver>/
cd wlroots-<newver>

# If the SONAME/API version changed (e.g. 0.19 -> 0.20):
#   - rename debian/libwlroots-<old>* files to libwlroots-<new>*
#   - update Package names + install paths + shlibs (blanket 0.19 -> 0.20, NOT changelog)

DEBEMAIL="paolo@pgentili.com" DEBFULLNAME="Paolo Gentili" \
  dch -v '<newver>-0ppa1~ubuntu26.04.1' -D resolute "New upstream release <newver>."

QUILT_PATCHES=debian/patches quilt push -a      # refresh/drop patches; then `quilt pop -a`

# Local build test:
dpkg-buildpackage -S -us -uc -d
cd ..
sg sbuild -c 'sbuild --chroot-mode=schroot -d resolute --arch=amd64 --chroot=resolute-amd64 wlroots_<newver>-0ppa1~ubuntu26.04.1.dsc'
lintian wlroots_<newver>-0ppa1~ubuntu26.04.1_amd64.changes

# Signed source upload (-d skips host build-dep check; Launchpad installs them):
cd wlroots-<newver>
debuild -S -sa -d -k'paolo@pgentili.com'
cd ..
dput ppa:pgentili/sway wlroots_<newver>-0ppa1~ubuntu26.04.1_source.changes
```

**Wait until wlroots shows _Published_** at
<https://launchpad.net/~pgentili/+archive/ubuntu/sway/+packages> before uploading sway.

### 2. sway

```bash
cd build
pull-lp-source sway resolute
cd sway-<oldver>/
uscan --download-version <newver> --destdir ..
cd ..
tar xf sway_<newver>.orig.tar.*
cp -r sway-<oldver>/debian sway-<newver>/
cd sway-<newver>

# Point build-dep at the matching wlroots (debian/control):
#   libwlroots-0.20-dev (>= 0.20)

DEBEMAIL="paolo@pgentili.com" DEBFULLNAME="Paolo Gentili" \
  dch -v '<newver>-0ppa1~ubuntu26.04.1' -D resolute "New upstream release <newver>."

QUILT_PATCHES=debian/patches quilt push -a      # drop patches merged upstream; `quilt pop -a`

# Local build test against the locally-built wlroots debs:
dpkg-buildpackage -S -us -uc -d
cd ..
sg sbuild -c "sbuild --chroot-mode=schroot -d resolute --arch=amd64 --chroot=resolute-amd64 --extra-package=$PWD sway_<newver>-0ppa1~ubuntu26.04.1.dsc"
lintian sway_<newver>-0ppa1~ubuntu26.04.1_amd64.changes

# Signed source upload:
cd sway-<newver>
debuild -S -sa -d -k'paolo@pgentili.com'
cd ..
dput ppa:pgentili/sway sway_<newver>-0ppa1~ubuntu26.04.1_source.changes
```

## Local runtime test

```bash
lxc launch ubuntu:26.04 sway-test
lxc file push build/libwlroots-0.20*_amd64.deb build/sway_1.12*_amd64.deb \
  build/sway-backgrounds*_all.deb sway-test/root/
lxc exec sway-test -- apt-get install -y /root/libwlroots-0.20*_amd64.deb \
  /root/sway_1.12*_amd64.deb /root/sway-backgrounds*_all.deb
lxc exec sway-test -- sway --version                       # -> sway version 1.12
lxc exec sway-test -- env WLR_BACKENDS=headless WLR_RENDERER=pixman \
  WLR_LIBINPUT_NO_DEVICES=1 XDG_RUNTIME_DIR=/tmp timeout 5 sway -c /dev/null
lxc delete -f sway-test
```

## Versioning

`<upstream>-0ppa1~ubuntu26.04.1` — the `~` suffix sorts below any future official Ubuntu
package, so users upgrade cleanly when the archive eventually ships a newer sway.
