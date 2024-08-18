#include "bn_core.h"
#include "bn_sprite_ptr.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_sprite_items_common_fixed_8x16_font.h"
#include "bn_string.h"
#include "bn_keypad.h"
#include "bn_vector.h"
//#include "bn_log.h"// plz remember including things we arent using wastes memory
#include "bn_camera_ptr.h"
#include "bn_rect_window.h"
#include "bn_bg_palettes.h"
#include "bn_regular_bg_position_hbe_ptr.h"

#include "bn_sprite_items_gem.h"
#include "bn_sprite_items_smallarrow.h"

#include "bn_music.h"
#include "bn_music_items.h"

#include "bn_sound_items.h"
#include "bn_sound.h"
#include "bn_sprite_items_sequencer_fixed_16x16_font.h" 
#include "bn_sprite_animate_actions.h"
#include "bn_sprite_items_modeselect.h"
#include "bn_sprite_items_dancerhead.h"
#include "bn_sprite_items_dancerbody.h"
#include "bn_regular_bg_items_checkerboard.h"
#include "bn_regular_bg_items_bgtop.h"


struct chord
{
    int root_note; // 1 to 7, 0 is empty
    int type;      // 0 is normal, 1 is 7th

    chord()
    {
        root_note = 0;
        type = 0;
    }
};

struct instrument
{
    // C3 to C5 pitch adjustment values assuming sample is C4
    float notes[37] = {.5, .529, .561, .595, .631, .669, .709, .751, .796, .843, .893, .946, 1, 1.058, 1.122, 1.189, 1.261, 1.337, 1.418, 1.505, 1.597, 1.695, 1.799, 1.909, 2, 2.116, 2.245, 2.378, 2.523, 2.674, 2.837, 3, 3.179, 3.391, 3.589, 3.818, 4};

    bool seventh = false;
    int sample_frames = 29;                // we use this to determine when to replay sample. different from sequencer play
    int arp_frame = 3;                    // tracks current arp note
    int max_frames = 30;            // how long until restart sample. 30 frames is 2 beats a second or 120 bpm
    const int arpeggio_separation = 20;   // how far apart the arp is?
    int arp_division = 128;
    int chord_comp[4] = {99, 99, 99, 99}; // the values of the current chord according to notes above

    bool arp = false;
    int key_root = 12; // 12 is c
    bool major = true; // false is minor
    int sample = 0;    // 0 is sample 0 and so on
    int tempo = 120;   // 120 is default, tempo can be between 60 - 240

    const int chord_y_pos = -50;

    // menu stuff
    bn::string<40> note_names[12] = {"C ", "C#", "D ", "D#", "E ", "F ", "F#", "G ", "G#", "A ", "A#", "B "};
    bn::string<32> sample_names[4] = {"Organic", "Steb", "Crystal","Electric"};
    bn::string<16> chord_names_major[8] = {"O", "A", "B", "C", "D", "E", "F", "G"};
    bn::string<16> chord_names_maj7[8] = {"O", "H", "I", "J", "K", "L", "M", "N"}; 
    bn::string<16> chord_names_minor[8] = {"O", "a", "b", "c", "d", "e", "f", "g"};
    bn::string<16> chord_names_min7[8] = {"O", "h", "i", "j", "k", "l", "m", "n"}; 
    bn::string<8> chord_name;

    bn::sound_item samples[4] = {bn::sound_items::t1c3,bn::sound_items::t2c3,bn::sound_items::t3c3,bn::sound_items::t4c3};
    
    bn::vector<bn::sprite_ptr, 32> text_sprites;
    bn::vector<bn::sprite_ptr, 5> test_txt;
    bn::vector<bn::sprite_ptr, 4> seven_text;

    bn::vector<bn::sprite_ptr, 32> key_text_sprites;
    bn::vector<bn::sprite_ptr, 32> sample_text_sprites;
    bn::vector<bn::sprite_ptr, 32> tempo_text_sprites;
    bn::vector<bn::sprite_ptr, 32> arp_text_sprites;

    bn::vector<bn::sprite_ptr, 32> sequence_text_sprites;

    bn::vector<bn::sprite_ptr, 32> sequence_label_sprites;

    bool chord_mode = true; // probably should not be a boolean but this tells us we are in chord playing mode rather than solo mode

    bool menu_mode = false;
    int menu_size = 4;

    int menu_index = 0;

    int sequence_edit_cursor = 0;
    int sequence_play_cursor = 0;
    
    int menu_start_y = -16;

    bn::sprite_ptr cursor = bn::sprite_items::gem.create_sprite(90, menu_start_y);

    bn::sprite_ptr seqcursor = bn::sprite_items::smallarrow.create_sprite(-56,75);

    bn::vector<chord, 8> sequence;

    bn::sprite_ptr sprite_mode_seq = bn::sprite_items::modeselect.create_sprite(-104, -72,1);
    bn::sprite_ptr sprite_mode_chord = bn::sprite_items::modeselect.create_sprite(-104, -72);


    // bn::sprite_ptr dancer_head = bn::sprite_items::dancerhead.create_sprite(-90, -47);
    
    // bn::sprite_ptr dancer_body = bn::sprite_items::dancerbody.create_sprite(-90, -20);




    void play_all();
    void set_chords(bn::sprite_text_generator text_generator);
    void handle_menu(bn::sprite_text_generator sequence_generator);
    void instrument_main(bn::sprite_text_generator text_generator, bn::sprite_text_generator sequence_generator);
    void set_chord_by_numeral(int numeral,bn::sprite_text_generator text_generator); // ok i feel like im passing this text generator too many times but whatever
    void set_single_chord(bool is_base_major, int offset, bool diminished,bn::sprite_text_generator text_generator);
    void play_single_note_by_numeral(int numeral); //for soloing
};

void instrument::play_all()
{
    
    if (chord_mode==false){ //should this go here? idk who cares it works
        seqcursor.set_x(-56 + (sequence_play_cursor * 16));
        seqcursor.set_visible(true);
    }
    else{
        seqcursor.set_visible(false);
    }
    
  //  BN_LOG(max_frames);
    if (arp)
    {
        if (chord_comp[0] != 99)
        {  
            if (sample_frames==0 && menu_mode==false){ 
                sequence_play_cursor = (sequence_play_cursor+1)%8;
            }
            // sample_frames = (sample_frames + 1) % arpeggio_separation * 3; // change this to mult by number of notes in chord
           sample_frames = (sample_frames + 1) % ( ((3600/(arp_division/4))/tempo) +1); //we could also distinguish whether arp is 16ths, 8ths, whatever. 
           if (sample_frames==0) {
            
                arp_frame=(arp_frame+1)%4;
                switch (arp_frame)
                {
                case 0: //changed from case arpeggio_separation
                    bn::sound::stop_all();
                    bn::sound::play(samples[sample], 1, notes[chord_comp[0]], .5);
                    
                   // dancer_body.set_horizontal_flip(true);
                    break;
                case 1: //changed from case arpeggio_separation * 2
                    bn::sound::stop_all();
                    bn::sound::play(samples[sample], 1, notes[chord_comp[1]], .5);
                    break;
                case 2: // changed from case arpeggio_separation * 3
                    bn::sound::stop_all();
                    bn::sound::play(samples[sample], 1, notes[chord_comp[2]], .5);
                  //  dancer_body.set_horizontal_flip(false);
                    break;
                case 3: // changed from case arpeggio_separation * 3
                    bn::sound::stop_all();
                    if (seventh)
                        bn::sound::play(samples[sample], 1, notes[chord_comp[3]], .5);
                    else bn::sound::play(samples[sample], 1, notes[chord_comp[1]], .5);
                    break;
                default:
                    break;
                }
           }

        }
    }
    else
    {
        
        if (sample_frames == 0)
        {
            if (menu_mode==false){ 
                sequence_play_cursor = (sequence_play_cursor+1)%8;
            }

            bn::sound::stop_all();
            if (chord_comp[0] != 99)
            {
                bn::sound::play(samples[sample], 1, notes[chord_comp[0]], .5);
                bn::sound::play(samples[sample], 1, notes[chord_comp[1]], .5);
                bn::sound::play(samples[sample], 1, notes[chord_comp[2]], .5);
                if (seventh)
                    bn::sound::play(samples[sample], 1, notes[chord_comp[3]], .5);
                
            }
        }
        
        sample_frames = (sample_frames + 1) % max_frames;
        
    }
}
void instrument::set_single_chord(bool is_base_major, int offset, bool diminished,bn::sprite_text_generator text_generator)
{
    test_txt.clear();
    if (diminished){
        chord_name = note_names[(key_root - 1) % 12] + "dim";
    }
    else{
        if (major)
            {
                if (is_base_major){
                    chord_name = note_names[(key_root + offset) % 12] + "maj";
                }
                else{
                    chord_name = note_names[(key_root + offset) % 12] + "min";
                }
            }
            else
            {
                if (is_base_major){
                    chord_name = note_names[(key_root + offset) % 12] + "min";
                }
                else{
                    chord_name = note_names[(key_root + offset) % 12] + "maj";
                }
            }

    }
    if (offset==99){
        chord_name = "-";
    }
    text_generator.generate(0, chord_y_pos, chord_name, test_txt);

    if (diminished){
        chord_comp[0] = key_root - 1;
        chord_comp[1] = key_root + 3 - 1;
        chord_comp[2] = key_root + 6 - 1;
        chord_comp[3] = key_root + 10 - 1; // 7th flat five????
    }
    else{
        chord_comp[0] = key_root + offset; //root

        if (is_base_major){ //third
            chord_comp[1] = key_root + 3 + major + offset;
        }
        else{
            chord_comp[1] = key_root + 4 - major + offset ;
        }
        
        chord_comp[2] = key_root + 7 + offset; //fifth

        if (is_base_major){ //seventh
            chord_comp[3] = key_root + 10 + major + offset; 
        }
        else{
            chord_comp[3] = key_root + 11 - major + offset;
        }
    }


}

void instrument::play_single_note_by_numeral(int numeral) {
    
    switch (numeral)
    {
    case 1:
        bn::music::set_volume(1);
        bn::music::set_pitch(notes[key_root-12]);
        break;
    case 2:
        bn::music::set_volume(1);
        bn::music::set_pitch(notes[key_root+2-12]);
        break;
    case 3:
        bn::music::set_volume(1);
        bn::music::set_pitch(notes[key_root+3+major-12]);
        break;
    case 4:
        bn::music::set_volume(1);
        bn::music::set_pitch(notes[key_root+5-12]);
        break;
    case 5:
        bn::music::set_volume(1);
        bn::music::set_pitch(notes[key_root+7-12]);
        break;
    case 6:
        bn::music::set_volume(1);
        bn::music::set_pitch(notes[key_root+8+major-12]);
        break;
    case 7: 
        bn::music::set_volume(1);
        bn::music::set_pitch(notes[key_root-2+major]);
        break;
    default:
        bn::music::set_volume(0);
        break;
    }
    
}

void instrument::set_chord_by_numeral(int numeral,bn::sprite_text_generator text_generator){
    switch (numeral)
    {
    case 1:
        set_single_chord(true,0,false,text_generator);
        break;
    case 2:
        set_single_chord(false,2,false,text_generator);
        break;
    case 3:
        set_single_chord(false,4,false,text_generator);
        break;
    case 4:
        set_single_chord(true,5,false,text_generator);
        break;
    case 5:
        set_single_chord(true,7,false,text_generator);
        break;
    case 6:
        set_single_chord(false,9,false,text_generator);
         //um so actually the fifth on this might randomly be 4 idk why or if that was a typo
         //also the root offset was originally 3. probably to not overextend on the pitches
        break;
    case 7:
        set_single_chord(false,1,true,text_generator);
        break;
    default:
    set_single_chord(false,99,false,text_generator);
        break;
    }
}

void instrument::set_chords(bn::sprite_text_generator text_generator)
{
    if (chord_mode==true){
        if (bn::keypad::any_pressed())
        {
            sample_frames = 0; // force sample to restart
            
            //  bn::sound::stop_all();

            if (bn::keypad::left_pressed())
            {   set_chord_by_numeral(1,text_generator);
            }
            else if (bn::keypad::up_pressed())
            {   set_chord_by_numeral(2,text_generator);
            }
            else if (bn::keypad::right_pressed())
            {   set_chord_by_numeral(3,text_generator);
                
            }
            else if (bn::keypad::down_pressed())
            {   set_chord_by_numeral(4,text_generator);
            }
            else if (bn::keypad::b_pressed())
            {   set_chord_by_numeral(5,text_generator);
            }
            else if (bn::keypad::a_pressed())
            {   set_chord_by_numeral(6,text_generator);
            }
            else if (bn::keypad::select_pressed())
            {   set_chord_by_numeral(7,text_generator);
            }
            else if (bn::keypad::start_pressed())
            {

                menu_mode = true;
                menu_index = 0;
                cursor.set_visible(true);
                cursor.set_y(menu_start_y);

            }

            /*
            else if (bn::keypad::l_pressed())
            {
                major = !major;
                text_sprites.clear();
                if (major)
                {
                    text_generator.generate(0, -60, note_names[key_root] + "maj", text_sprites);
                    // check if chord name has dim or this will be wrong
                    chord_comp[1] = chord_comp[1] + 1;
                }
                else
                {
                    text_generator.generate(0, -60, note_names[key_root] + "min", text_sprites);
                    // check if chord name has dim or this will be wrong
                    chord_comp[1] = chord_comp[1] + 1;
                }
            }
            */
            if (bn::keypad::l_pressed())
            {
                //BN_LOG("chordmode swap");
                chord_mode = false;
                sprite_mode_chord.set_visible(false);
                
                
            }
            if (bn::keypad::r_held())
            {
                seventh = true;
                
                text_generator.generate(22, chord_y_pos-3, "7", seven_text);
            }
            else
            {
                seventh = false;
                seven_text.clear();
            }
        }
    }
    else{
       
       // sequence_play_cursor = (sequence_play_cursor+1)%8;

        set_chord_by_numeral(sequence[sequence_play_cursor].root_note,text_generator);
        if (sequence[sequence_play_cursor].type == 1){
            seventh=true;
            seven_text.clear();
            text_generator.generate(22, chord_y_pos-3, "7", seven_text);
        }
        else{
            seventh=false;
            seven_text.clear();
        }
      //  set_chord_by_numeral(2,text_generator);
        if (bn::keypad::left_held())
        {   
            play_single_note_by_numeral(1);
        }
        else if (bn::keypad::up_held())
        {   play_single_note_by_numeral(2);
        }
        else if (bn::keypad::right_held())
        {   play_single_note_by_numeral(3);
        }
        else if (bn::keypad::down_held())
        {   play_single_note_by_numeral(4);
        }
        else if (bn::keypad::b_held())
        {   play_single_note_by_numeral(5);
        }
        else if (bn::keypad::a_held())
        {   play_single_note_by_numeral(6);
        }
        else if (bn::keypad::select_held())
        {   play_single_note_by_numeral(7);
        }

        else if (bn::keypad::start_pressed())
        {
            bn::music::set_volume(0);
            menu_mode = true;
            menu_index = 0;
            cursor.set_visible(true);
            cursor.set_y(menu_start_y);
        }
        
        else {
            bn::music::set_volume(0);
        }
        if (bn::keypad::l_pressed())
        {
            bn::music::set_volume(0);
            chord_mode = true;
            sprite_mode_chord.set_visible(true);

        }
    }


    // bn::core::update();
}

void instrument::handle_menu(bn::sprite_text_generator sequence_generator)
{

    if (bn::keypad::start_pressed())
    {
       // menu_mode = !menu_mode;
        menu_mode = false;
        menu_index = menu_size + 1;
        cursor.set_visible(false);
        cursor.set_y(menu_start_y);
    }

    if (!menu_mode) ////Do instrument playing stuff
    {


        if (bn::keypad::r_held() && !seventh)
        {
            seventh = true;
        }
        else if (seventh)
        {
            seventh = false;
        }
        // put the rest of the instrument logic here
    }
    // Up and down controls on the menu
    else if (bn::keypad::up_pressed())
    {
        if (menu_index == 0)
        {
            menu_index = menu_size;
            cursor.set_y(menu_start_y + menu_size * 16);
        }
        else
        {
            menu_index--;
            cursor.set_y(cursor.y() - 16);
        }
    }
    else if (bn::keypad::down_pressed())
    {
        if (menu_index == menu_size)
        {
            menu_index = 0;
            cursor.set_y(menu_start_y);
        }
        else
        {
            menu_index++;
            cursor.set_y(cursor.y() + 16);
        }
    }

    // Checks what index the menu is on
    switch (menu_index)
    {
    case 0: // Changing the key
        if (bn::keypad::left_pressed())
        {
            if (major)
            {
                if (key_root == 12)
                {
                    key_root = 24;
                }
                else
                {
                    key_root--;
                }
                major = false;
            }
            else
            {
                major = true;
            }
        }
        else if (bn::keypad::right_pressed())
        {
            if (major)
            {
                major = false;
            }
            else
            {
                if (key_root == 24)
                {
                    key_root = 12;
                }
                else
                {
                    key_root++;
                }
                major = true;
            }
        }
        break;

    case 1: // Changing the selected sample
        if (bn::keypad::left_pressed())
        {
            if (sample == 0)
            {
                sample = 3;
            }
            else
            {
                sample--;
            }
        }
        else if (bn::keypad::right_pressed())
        {
            if (sample == 3)
            {
                sample = 0;
            }
            else
            {
                sample++;
            }
        }
        break;

     case 2: // Changing the Tempo
        if (bn::keypad::l_pressed())
        {
            if (tempo <= 60)
            {
                tempo = 240;
            }
            else
            {
                tempo -= 1;
            }
        }
        if (bn::keypad::left_pressed())
        {
            if (tempo <= 60)
            {
                tempo = 240;
            }
            else
            {
                tempo -= 5;
            }
        }
        else if (bn::keypad::r_pressed())
        {
            if (tempo  >= 240)
            {
                tempo = 60;
            }
            else
            {
                tempo += 1;
            }
        }
        else if (bn::keypad::right_pressed())
        {
            if (tempo >= 240)
            {
                tempo = 60;
            }
            else
            {
                tempo += 5;
            }
        }

        max_frames = 3600/tempo;
        break;

    case 3: // Arpeggiate
        if (bn::keypad::left_pressed())
        {
            switch (arp_division)
            {
                case 4:
                    arp_division = 128;
                    arp = false;
                    break;
                case 8:
                    arp_division = 4;
                    break;
                case 16:
                    arp_division = 8;
                    break;
                case 32:
                    arp_division = 16;
                    break;
                case 64:
                    arp_division = 32;
                    break;
                case 128:
                    if (arp)
                    {
                        arp_division = 64;
                    }
                    else
                    {
                        arp= true;
                    }
                    break;
                default:
                    arp_division = 128;
                    arp = false;
                    break;
            }
        }
        else if (bn::keypad::right_pressed())
        {
            switch (arp_division)
            {
                case 4:
                    arp_division = 8;
                    break;
                case 8:
                    arp_division = 16;
                    break;
                case 16:
                    arp_division = 32;
                    break;
                case 32:
                    arp_division = 64;
                    break;
                case 64:
                    arp_division = 128;
                    break;
                case 128:
                    if (arp)
                    {
                        arp = false;
                    }
                    else
                    {
                        arp = true;
                        arp_division = 4;
                    }
                    break;
                default:
                    arp_division = 128;
                    arp = false;
                    break;
            }
        }
        break;
    case 4: // Sequence Editing
        if (bn::keypad::left_pressed())
        {
            if (sequence_edit_cursor == 0)
            {
                sequence_edit_cursor = 7;
            }
            else
            {
                sequence_edit_cursor--;
            }
        }
        else if (bn::keypad::right_pressed())
        {
            if (sequence_edit_cursor == 7)
            {
                sequence_edit_cursor = 0;
            }
            else
            {
                sequence_edit_cursor++;
            }
        }
        else if (bn::keypad::a_pressed())
        {
            if (sequence[sequence_edit_cursor].root_note == 7)
            {
                if (sequence[sequence_edit_cursor].type == 0)
                {
                    sequence[sequence_edit_cursor].type = 1;
                    sequence[sequence_edit_cursor].root_note = 1;
                }
                else
                {
                    sequence[sequence_edit_cursor].type = 0;
                    sequence[sequence_edit_cursor].root_note = 0;
                }
            }
            else
            {
                sequence[sequence_edit_cursor].root_note += 1;
            }
        }
        else if (bn::keypad::b_pressed())
        {
           if (sequence[sequence_edit_cursor].root_note == 0)
            {
                if (sequence[sequence_edit_cursor].type == 1)
                {
                    sequence[sequence_edit_cursor].type = 0;
                    sequence[sequence_edit_cursor].root_note = 7;
                }
                else
                {
                    sequence[sequence_edit_cursor].type = 1;
                    sequence[sequence_edit_cursor].root_note = 7;
                }
            }
            else
            {
                sequence[sequence_edit_cursor].root_note -= 1;
            }
        }
        else if (bn::keypad::r_pressed())
        {
            if (sequence[sequence_edit_cursor].type == 1)
            {
                sequence[sequence_edit_cursor].type = 0;
            }
            else
            {
                sequence[sequence_edit_cursor].type = 1;
            }
        }
        else if (bn::keypad::l_pressed())//clear note
        {
            sequence[sequence_edit_cursor].root_note=0;
            sequence[sequence_edit_cursor].type = 0;
        }
        break;

    default:
        break;
    }

    // Updates the sequencer display
    sequence_text_sprites.clear();

    bn::string<100> sequence_text = "";

    for (int i = 0; i < 8; i++)
    {
        bn::string<10> sequence_chord_name = "";

        if (sequence[i].type == 0) //NEW SHIT
        {
            if(major){
                sequence_chord_name += chord_names_major[sequence[i].root_note]; 
            }
            else{
                sequence_chord_name += chord_names_minor[sequence[i].root_note]; 
            }
            
        }
        else
        {
            if(major){
                sequence_chord_name += chord_names_maj7[sequence[i].root_note]; 
                
            }
            else{
                sequence_chord_name += chord_names_min7[sequence[i].root_note]; 
            }
        }

        if (sequence_edit_cursor == i && menu_index == menu_size)
        {
            sequence_text += "<" + sequence_chord_name + ">"; //NEW SHIT
        }
        else
        {
            sequence_text += sequence_chord_name; //NEW SHIT
        }

        // NEW SHIT (stuff was removed here)
    }

    sequence_generator.generate(0, menu_start_y + menu_size * 16 + 16, sequence_text, sequence_text_sprites); //NEW SHIT
}

void instrument::instrument_main(bn::sprite_text_generator text_generator, bn::sprite_text_generator sequence_generator) 
{

    bn::regular_bg_ptr checker_bg =bn::regular_bg_items::checkerboard.create_bg(0, 0);  
    bn::regular_bg_ptr regular_bg =bn::regular_bg_items::bgtop.create_bg(8, 48);
    text_generator.set_center_alignment();
    sequence_generator.set_center_alignment();
    bn::bg_palettes::set_transparent_color(bn::color(3, 3, 15));
    bn::music::play(bn::music_items::solo);
    bn::music::set_volume(0);
    while (sequence.size() < 8)
    {
        sequence.push_back(chord());
    }

    cursor.set_visible(menu_mode);
    
    text_generator.generate(0, menu_start_y + menu_size * 16, "SEQUENCE:", sequence_label_sprites);

    while (true)
    {
        play_all();
        if (menu_mode == true)
        {
            handle_menu( sequence_generator); 
        }

        else if (menu_mode == false)
        {
            set_chords(text_generator);
        }
        key_text_sprites.clear();
        sample_text_sprites.clear();
        tempo_text_sprites.clear();
        arp_text_sprites.clear();

        text_generator.generate(0, menu_start_y, "Key: < " + note_names[key_root % 12] + (major ? " MAJ" : " min") + " >", key_text_sprites);
        text_generator.generate(0, menu_start_y + 16, "Sample: < " + sample_names[sample] + " >", sample_text_sprites);
        text_generator.generate(0, menu_start_y + 16 + 16, "BPM: " + bn::to_string<12>(tempo), tempo_text_sprites);
        text_generator.generate(0, menu_start_y + 16 + 16 + 16, "Arpeggio: < " + (arp ? "1/" + bn::to_string<20>(arp_division): "OFF") + " >", arp_text_sprites);

        

        bn::core::update();
    }
}

int main()
{
    bn::core::init();
   
    bn::sprite_text_generator menu_generator(bn::sprite_font(bn::sprite_items::common_fixed_8x16_font));
    bn::sprite_text_generator sequencer_generator(bn::sprite_font(bn::sprite_items::sequencer_fixed_16x16_font)); //NEW SHIT
    instrument the_instrument = instrument();
    the_instrument.instrument_main(menu_generator, sequencer_generator);

    

}