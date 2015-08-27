Smilodon - SabreDAV for Drupal
=======

The goal of this project is to integrate the CardDAV and, at a later stage, CalDAV portions of SabreDAV (http://sabre.io)
into drupal.

Following drupal best practices, SabreDAV will *NOT* be included into this package but added via the Libraries module 
(http://drupal.org/project/libraries) to avoid complications.

Since there are plenty of great modules out there that provide addressbook-like functionality for drupal, there is no
point duplicating anything on the frontend side of things.
For the addresses, we will rely on addressfield (http://drupal.org/project/addressfield). In order to avoid duplications
and redundancies, we need to go very deep into drupal's structure. The idea is to leverage drupal's Field API and divert
any CRUD operations away from the normal SQL storage engine, piping it through SabreDAV\vobject instead.

Since CardDAV is the easier of the two, I will start out with this one, and once the storage and DAV-bits are working,
I'll move on to CardDAV (something tells me, this is going to be painful).

Happy hacking!
