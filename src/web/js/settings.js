export default () => ({
    settings: {
        mode: 2,
        dateFormat: "{ddd} {dd}/{MM}",
        timeFormat: "{HH}:{mm}",
    },
    errors: {},
    timezones: {},

    get header() {
        return this.settings.name || "Split Flap";
    },

    get addressArray() {
        return (
            this.settings.moduleAddresses
                ?.split(",")
                .map((s) => s.trim()) || []
        );
    },
    setAddress(index, value) {
        const arr = this.addressArray;
        arr[index] = value;
        this.settings.moduleAddresses = arr.join(",");
    },

    get offsetArray() {
        return (
            this.settings.moduleOffsets?.split(",").map((s) => s.trim()) ||
            []
        );
    },
    setOffset(index, value) {
        const arr = this.offsetArray;
        arr[index] = value;
        this.settings.moduleOffsets = arr.join(",");
    },

    loadSettings() {
        fetch("/settings")
            .then((res) => res.json())
            .then((data) => {
                Object.assign(this.settings, data);
            })
            .catch(() =>
                this.showDialog("Failed to load settings", "error", true),
            )
            .finally(() => {
                this.loading.settings = false;
            });
    },

    loadTimezones() {
        fetch("/timezones.json")
            .then((res) => res.json())
            .then((data) => {
                this.timezones = data;
            })
            .catch(() =>
                this.showDialog(
                    "Failed to load timezones. Refresh the page.",
                    "error",
                    true,
                ),
            )
            .finally(() => (this.loading.timezones = false));
    },

    save() {
        this.saving = true;
        this.errors = {};

        fetch("/settings", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify(this.settings),
        })
            .then((res) => res.json())
            .then((data) => {
                this.errors = data.errors || {};
                this.showDialog(data.message, data.type, data.persistent);
                if (data.redirect) {
                    setTimeout(() => {
                        window.location.href = data.redirect;
                    }, 10000);
                }
            })
            .catch(() =>
                this.showDialog("Failed to save settings.", "error"),
            )
            .finally(() => (this.saving = false));
    },

    reset() {
        if (
            confirm("Are you sure you want to reset settings to defaults?")
        ) {
            fetch("/settings/reset", { method: "POST" })
                .then((res) => res.json())
                .then((data) => {
                    this.showDialog(
                        data.message,
                        data.type,
                        data.persistent,
                    );
                    this.loadSettings();
                })
                .catch(() => {
                    this.showDialog("Failed to reset settings.", "error");
                });
        }
    },
});
