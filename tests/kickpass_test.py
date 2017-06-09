#
# Copyright (c) 2015 Paul Fariello <paul@fariello.eu>
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#
import logging
import tempfile
import os
import unittest
import kickpass

def prompt(ctx, confirm, prompt):
    return b"master password"

class TestKickpass(unittest.TestCase):


    @classmethod
    def setUpClass(cls):
        logging.getLogger().setLevel(logging.INFO)


    def setUp(self):
        self.home = tempfile.TemporaryDirectory()
        os.environ['HOME'] = self.home.name


    def tearDown(self):
        self.home.cleanup()


    def test_get_context(self):
        # When
        ctx = kickpass.Context(prompt=prompt)

        # Then
        self.assertIsNotNone(ctx)


    def test_init_workspace(self):
        # Given
        ctx = kickpass.Context(prompt=prompt)

        # When
        ctx.init()

        # Then
        self.assertTrue(os.path.isdir(os.path.join(self.home.name, ".kickpass")))


    def test_create_safe(self):
        # Given
        ctx = kickpass.Context(prompt=prompt)
        ctx.init()
        safe = kickpass.Safe(ctx, "test")

        # When
        safe.open(create=True)

        # Then
        safe.close()


    def test_open_inexistant_safe_raise_exception(self):
        # Given
        ctx = kickpass.Context(prompt=prompt)
        ctx.init()
        safe = kickpass.Safe(ctx, "test")
        exception = None

        # When
        try:
            safe.open()
        except kickpass.Exception as e:
            exception = e


        # Then
        self.assertIsNotNone(exception)


    def test_edit_safe(self):
        # Given
        ctx = kickpass.Context(prompt=prompt)
        ctx.init()
        safe = kickpass.Safe(ctx, "test")
        safe.open(create=True)

        # When
        safe.password = b"test password"
        safe.metadata = b"metadata"
        safe.save()
        safe.close()

        # Then
        safe.open()
        self.assertEqual(safe.password, b"test password")
        self.assertEqual(safe.metadata, b"metadata")

        safe.close()


    def test_closed_safe_has_no_password(self):
        # Given
        ctx = kickpass.Context(prompt=prompt)
        ctx.init()
        safe = kickpass.Safe(ctx, "test")
        safe.open(create=True)
        safe.password = b"test password"
        safe.metadata = b"metadata"
        safe.save()

        # When
        safe.close()

        # Then
        self.assertIsNone(safe.password)
        self.assertIsNone(safe.metadata)


if __name__ == '__main__':
        unittest.main()
