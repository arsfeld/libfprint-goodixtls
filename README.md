> [!IMPORTANT]
> ## This is a fork, not upstream libfprint
>
> **Upstream [libfprint][Upstream] `v1.94.100`, plus the community `goodixtls`
> drivers for Goodix TLS fingerprint sensors** — `27c6:5110`, `27c6:521d` and
> `27c6:538d`. All three are on upstream's *unsupported devices* list, so
> stock libfprint will not drive them.
>
> The drivers come from [infinytum/libfprint][Source], which stopped at
> libfprint `v1.94.1` in 2021 and had drifted five years behind upstream. This
> fork carries the same drivers on a current base, so it builds against a
> current `fprintd` without patching version strings or back-filling enums.
>
> ### What changed relative to the source fork
>
> - Rebuilt on `v1.94.100` instead of `v1.94.1`. The driver sources are imported
>   verbatim; only the build-system integration was rewritten, since upstream
>   merged `default_drivers` and `driver_helper_mapping` into a single
>   `drivers_info` dict.
> - **Fixed the incompatible pointer types** that downstream packages worked
>   around with `-Wno-incompatible-pointer-types`. These were type errors, not
>   logic errors, so generated code is unchanged — but the suppression is no
>   longer needed, and no longer hides anything new.
> - **`goodixtls52xd` accepts any `GFUSB_GM168SEC_APP_` firmware.** The driver
>   gated on an exact `strcmp` against `…_10019`, the build the public firmware
>   dump ships. Windows provisions `…_10034` on the same hardware, so a sensor
>   that has ever run the Windows stack failed the check and aborted before
>   reaching the PSK handshake.
> - `data/autosuspend.hwdb` is regenerated from the generator rather than
>   hand-edited, so it stays consistent with `fprint-list-udev-hwdb.c` and with
>   `tests/test-generated-hwdb.sh`.
> - Fixed an unrelated upstream bug: `v1.94.100` cannot configure with
>   `-Dintrospection=false`, because `tests/meson.build` iterates a dict with a
>   single loop variable.
>
> ### Status — read before relying on this
>
> - **Only `goodixtls52xd` (`27c6:521d`) is built and exercised**, on a single
>   machine. `goodixtls511` and `goodixtls53xd` are carried for provenance: they
>   compile, and that is all that is known about them here.
> - **These drivers terminate the sensor's TLS channel with a pre-shared key,
>   and expect that PSK to be all zero bytes.** Anyone with brief physical
>   access to the USB device can therefore capture the image stream. Weigh that
>   before wiring fingerprint auth into `sudo` or `polkit`.
> - Known unfixed bug, inherited and left alone deliberately: in `goodix52xd.c`
>   and `goodix53xd.c`, `guint16 payload = {0x05, 0x03};` initializes a *scalar*
>   from a braced list, so it is `0x0005` and the `0x03` is silently dropped.
>   Fixing it would change what is written to sensor register `556`, which needs
>   testing against real hardware first.
> - Not upstreamable as-is, and not submitted. No support is offered.
>
> ### Building just this driver
>
> ```sh
> meson setup build -Ddrivers=goodixtls52xd -Dintrospection=false -Ddoc=false -Dgtk-examples=false
> ninja -C build
> ```
>
> Packaged for NixOS in [arsfeld/nixos][Packaging].
>
> [Upstream]: https://gitlab.freedesktop.org/libfprint/libfprint
> [Source]: https://github.com/infinytum/libfprint
> [Packaging]: https://github.com/arsfeld/nixos/tree/master/packages/libfprint-goodix-521d

---


<div align="center">

# LibFPrint

*LibFPrint is part of the **[FPrint][Website]** project.*

<br/>

[![Button Website]][Website]
[![Button Documentation]][Documentation]

[![Button Supported]][Supported]
[![Button Unsupported]][Unsupported]

[![Button Contribute]][Contribute]
[![Button Contributors]][Contributors]

</div>

## History

**LibFPrint** was originally developed as part of an
academic project at the **[University Of Manchester]**.

It aimed to hide the differences between consumer
fingerprint scanners and provide a single uniform
API to application developers.

## Goal

The ultimate goal of the **FPrint** project is to make
fingerprint scanners widely and easily usable under
common Linux environments.

## License

`Section 6` of the license states that for compiled works that use
this library, such works must include **LibFPrint** copyright notices
alongside the copyright notices for the other parts of the work.

**LibFPrint** includes code from **NIST's** **[NBIS]** software distribution.

We include **Bozorth3** from the **[US Export Controlled]**
distribution, which we have determined to be fine
being shipped in an open source project.

## Get in *touch*

 - [IRC] - `#fprint` @ `irc.oftc.net`
 - [Matrix] - `#fprint:matrix.org` bridged to the IRC channel
 - [MailingList] - low traffic, not much used these days

<br/>

<div align="right">

[![Badge License]][License]

</div>


<!----------------------------------------------------------------------------->

[Documentation]: https://fprint.freedesktop.org/libfprint-dev/
[Contributors]: https://gitlab.freedesktop.org/libfprint/libfprint/-/graphs/master
[Unsupported]: https://gitlab.freedesktop.org/libfprint/wiki/-/wikis/Unsupported-Devices
[Supported]: https://fprint.freedesktop.org/supported-devices.html
[Website]: https://fprint.freedesktop.org/
[MailingList]: https://lists.freedesktop.org/mailman/listinfo/fprint
[IRC]: ircs://irc.oftc.net:6697/#fprint
[Matrix]: https://matrix.to/#/#fprint:matrix.org

[Contribute]: ./HACKING.md
[License]: ./COPYING

[University Of Manchester]: https://www.manchester.ac.uk/
[US Export Controlled]: https://fprint.freedesktop.org/us-export-control.html
[NBIS]: http://fingerprint.nist.gov/NBIS/index.html


<!---------------------------------[ Badges ]---------------------------------->

[Badge License]: https://img.shields.io/badge/License-LGPL2.1-015d93.svg?style=for-the-badge&labelColor=blue


<!---------------------------------[ Buttons ]--------------------------------->

[Button Documentation]: https://img.shields.io/badge/Documentation-04ACE6?style=for-the-badge&logoColor=white&logo=BookStack
[Button Contributors]: https://img.shields.io/badge/Contributors-FF4F8B?style=for-the-badge&logoColor=white&logo=ActiGraph
[Button Unsupported]: https://img.shields.io/badge/Unsupported_Devices-EF2D5E?style=for-the-badge&logoColor=white&logo=AdBlock
[Button Contribute]: https://img.shields.io/badge/Contribute-66459B?style=for-the-badge&logoColor=white&logo=Git
[Button Supported]: https://img.shields.io/badge/Supported_Devices-428813?style=for-the-badge&logoColor=white&logo=AdGuard
[Button Website]: https://img.shields.io/badge/Homepage-3B80AE?style=for-the-badge&logoColor=white&logo=freedesktopDotOrg
