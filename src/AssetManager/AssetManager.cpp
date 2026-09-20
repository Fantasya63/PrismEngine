#include "AssetManager.h"

#include <tiny_gltf_v3.h>


namespace AssetManager
{
    void LoadModel(const std::filesystem::path& path)
    {
        tg3_parse_options opts;
        tg3_error_stack errors;
        tg3_model model;

        tg3_parse_options_init(&opts);
        tg3_error_stack_init(&errors);

        tg3_error_code err = tg3_parse_file(&model, &errors, "scene.gltf", 10, &opts);
        if (err != TG3_OK) {
            for (uint32_t i = 0; i < errors.count; i++) {
                fprintf(stderr, "[%d] %s\n", (int)errors.entries[i].severity,
                        errors.entries[i].message ? errors.entries[i].message : "(null)");
            }
        }
        // ... use model ...
        

        tg3_model_free(&model);
        tg3_error_stack_free(&errors);

    }
}