# AnerkennungsDB

## About
AnerkennungsDB is a Qt-and SQLite-based database GUI application to keep track of the connection between transferred courses and study modules. The awarded points per course and module are expected in the European Credit Transfer System (ECTS).

## Features
AnerkennungsDB is builds on top of a SQLite database.

It offers support for
* Addition/removal/editing of courses and modules of study programs
* Addition and removal of links between courses transferred to modules, including plausibility check on the awarded ECTS points
* Views of courses per module and modules per course
* Searching in all tables
* Printing
* Exporting and importing the raw SQLite database
* Exporting to CSV format, the entire connections as a single file, or each table individually (packed as zip archive)
* Configuration of the database location; can be set to any directory allowing for a portable version
* Localization (currently only German and English are available)

## Compilation Requirements
It requires access to the [QuaZIP](https://github.com/stachenov/quazip) library, either installed system-wide or in a local directory
See the comments at the end of [anerkennungen.pro](./anerkennungen.pro)

## Translations
Currently, German is the only language for which a translation is provided. Translations into other languages are always welcome.

## License
GPL version 3 or later
