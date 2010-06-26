#include "unufo_patch.h"

#include "unufo_geometry.h"
#include "unufo_pixel.h"

using namespace std;

namespace unufo {

void transfer_patch(const Bitmap<uint8_t>& data, int bpp,
        const Bitmap<uint8_t>& confidence_map,
        const Matrix<Coordinates>& transfer_map,
        const Matrix<int>& transfer_belief,
        const Coordinates& position, const Coordinates& source,
        int belief, const vector<int>& best_color_diff)
{
    for(int j=0; j<bpp; j++) {
        int new_color = data.at(source)[j] + best_color_diff[j];
        data.at(position)[j] = new_color;
    }
    // TODO: better confidence transfer
    *confidence_map.at(position) = max(10, *confidence_map.at(source) - 5); 
    *transfer_map.at(position) = source;
    *transfer_belief.at(position) = belief;
}

int get_difference_color_adjustment(const Bitmap<uint8_t>& data,
        const Bitmap<uint8_t>& confidence_map,
        int comp_patch_radius,
        const Coordinates& candidate,
        const Coordinates& position,
        vector<int>& best_color_diff,
        int best, int bpp,
        int max_adjustment, bool equal_adjustment)
{
    int max_defined_size = 4*(2*comp_patch_radius + 1)*(2*comp_patch_radius + 1);
    int defined_only_near_pos;
    int confidence_sum;

    int accum[4] = {0, 0, 0, 0};
    // *((int64_t*)accum) = 0LL;
    // *((int64_t*)accum+1) = 0LL;

    uint8_t defined_near_pos [max_defined_size];
    uint8_t defined_near_cand[max_defined_size];

    int compared_count = collect_defined_in_both_areas(data, confidence_map,
            position, candidate,
            comp_patch_radius,
            defined_near_pos, defined_near_cand,
            defined_only_near_pos, confidence_sum);

    if (!compared_count)
        return best;

    int sum = defined_only_near_pos*max_diff;

    uint8_t* def_n_p = defined_near_pos;
    uint8_t* def_n_c = defined_near_cand;

    for (int i=0; i<compared_count; ++i) {
        for (int j=0; j<4; ++j)
            accum[j] += (int(def_n_p[j]) - int(def_n_c[j]));
        def_n_p += 4;
        def_n_c += 4;
    }

    for(int j=0; j<4; ++j) {
        accum[j] /= compared_count;
        if (accum[j] < -max_adjustment)
            accum[j] = -max_adjustment;

        if (accum[j] > max_adjustment)
            accum[j] = max_adjustment;
    }

    if (equal_adjustment) {
        int color_diff_sum = 0;
        for(int j=0; j<4; ++j)
            color_diff_sum += accum[j];
        for(int j=0; j<4; ++j)
            accum[j] = color_diff_sum/bpp;
    }

    def_n_p = defined_near_pos;
    def_n_c = defined_near_cand;
    for (int i=0; i<compared_count; ++i) {
        for (int j=0; j<4; ++j) {
            int c = int(def_n_c[j]) + accum[j];
            // do not allow color clipping
            if (c < 0 || c > 255)
                return best+1;
            sum += pixel_diff(c, def_n_p[j]);
        }
        def_n_p += 4;
        def_n_c += 4;
    }

    if (sum < best)
       best_color_diff.assign(accum, accum+bpp);
    return sum*(266 - confidence_sum/compared_count);
}

int get_difference(const Bitmap<uint8_t>& data,
        const Bitmap<uint8_t>& confidence_map,
        int comp_patch_radius,
        const Coordinates& candidate,
        const Coordinates& position, int best)
{
    int max_defined_size = 4*(2*comp_patch_radius + 1)*(2*comp_patch_radius + 1);
    int defined_only_near_pos;
    int confidence_sum;

    uint8_t defined_near_pos [max_defined_size];
    uint8_t defined_near_cand[max_defined_size];

    int compared_count = collect_defined_in_both_areas(data, confidence_map,
            position, candidate,
            comp_patch_radius,
            defined_near_pos, defined_near_cand,
            defined_only_near_pos, confidence_sum);

    if (compared_count) {
        int sum = defined_only_near_pos*max_diff;

        uint8_t* def_n_p = defined_near_pos;
        uint8_t* def_n_c = defined_near_cand;
        for (int i=0; i<compared_count; ++i) {
            for (int j=0; j<4; ++j) {
                sum += pixel_diff(def_n_c[j], def_n_p[j]);
            }

            def_n_p += 4;
            def_n_c += 4;
        }

        return sum*(266 - confidence_sum/compared_count);
    } else
        return best;
}

}
