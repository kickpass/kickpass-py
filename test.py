import kickpass

def prompt(ctx, confirm, prompt):
    return "1234"

ctx = kickpass.Context(prompt)
safe = kickpass.Safe(ctx, "toto")
safe.open()
