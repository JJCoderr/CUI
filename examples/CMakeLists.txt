function(cui_example name)
    add_executable(${name} ${ARGN})
    target_link_libraries(${name} PRIVATE cui::cui)
    cui_app_target(${name})   # no console on Windows, and only the library code the example calls
    if(APPLE)
        set_target_properties(${name} PROPERTIES MACOSX_BUNDLE OFF)
    endif()
endfunction()

cui_example(hello hello.c)
# The same window with its interface in examples/hello.xml (see docs/xml.md).
cui_example(helloxml hello_xml.c)
target_compile_definitions(helloxml PRIVATE CUI_HELLO_XML_PATH="${CMAKE_CURRENT_SOURCE_DIR}/hello.xml")
cui_example(gallery gallery.c gallery_ui.c gallery_more.c)
# The same gallery with its interface in examples/gallery.xml (see docs/xml.md).
# It shares the gallery's state and its page list with gallery.c, so the two
# cannot drift apart: tests/xml_gallery_shots.c draws both and compares.
cui_example(galleryxml gallery_xml.c gallery_xml_ui.c gallery_ui.c gallery_more.c)
target_compile_definitions(galleryxml PRIVATE CUI_GALLERY_XML_PATH="${CMAKE_CURRENT_SOURCE_DIR}/gallery.xml")
cui_example(dashboard dashboard.c)
# The same dashboard with its interface in examples/dashboard.xml (the orders
# and the figures of a range stay in C).
cui_example(dashboardxml dashboard_xml.c)
target_compile_definitions(dashboardxml PRIVATE CUI_DASHBOARD_XML_PATH="${CMAKE_CURRENT_SOURCE_DIR}/dashboard.xml")
cui_example(settings settings.c)
# The same account with its interface in examples/settings.xml (the state stays C).
cui_example(settingsxml settings_xml.c)
target_compile_definitions(settingsxml PRIVATE CUI_SETTINGS_XML_PATH="${CMAKE_CURRENT_SOURCE_DIR}/settings.xml")
cui_example(notes notes.c)
# The same notes with its interface in examples/notes.xml (the rows stay in C).
cui_example(notesxml notes_xml.c)
target_compile_definitions(notesxml PRIVATE CUI_NOTES_XML_PATH="${CMAKE_CURRENT_SOURCE_DIR}/notes.xml")
cui_example(mail mail.c)
# The same inbox with its interface in examples/mail.xml (the messages and the
# rows of the list stay in C).
cui_example(mailxml mail_xml.c)
target_compile_definitions(mailxml PRIVATE CUI_MAIL_XML_PATH="${CMAKE_CURRENT_SOURCE_DIR}/mail.xml")
cui_example(tracker tracker.c)
# The same tracker with its interface in examples/tracker.xml (the issues, the
# glyphs and the sheets stay in C).
cui_example(trackerxml tracker_xml.c)
target_compile_definitions(trackerxml PRIVATE CUI_TRACKER_XML_PATH="${CMAKE_CURRENT_SOURCE_DIR}/tracker.xml")
cui_example(music music.c)
# The same player with its interface in examples/music.xml (see docs/xml.md).
cui_example(musicxml music_xml.c)
target_compile_definitions(musicxml PRIVATE CUI_MUSIC_XML_PATH="${CMAKE_CURRENT_SOURCE_DIR}/music.xml")
cui_example(finance finance.c)
# The same overview with its interface in examples/finance.xml (the transactions
# and the painted card stay in C).
cui_example(financexml finance_xml.c)
target_compile_definitions(financexml PRIVATE CUI_FINANCE_XML_PATH="${CMAKE_CURRENT_SOURCE_DIR}/finance.xml")
cui_example(assistant assistant.c)
# The same chat with its interface in examples/assistant.xml (the answer and the
# conversation stay in C).
cui_example(assistantxml assistant_xml.c)
target_compile_definitions(assistantxml PRIVATE CUI_ASSISTANT_XML_PATH="${CMAKE_CURRENT_SOURCE_DIR}/assistant.xml")
cui_example(todo todo.c)
# The same to-do list with its interface in examples/todo.xml.
cui_example(todoxml todo_xml.c)
target_compile_definitions(todoxml PRIVATE CUI_TODO_XML_PATH="${CMAKE_CURRENT_SOURCE_DIR}/todo.xml")
cui_example(caffeine caffeine.c)
# The same picture with its layout in examples/caffeine.xml (the model stays C).
cui_example(caffeinexml caffeine_xml.c)
target_compile_definitions(caffeinexml PRIVATE CUI_CAFFEINE_XML_PATH="${CMAKE_CURRENT_SOURCE_DIR}/caffeine.xml")
# The XML demo: the interface is examples/xml_demo.xml; the C file only binds
# values and actions, and reloads the file when it changes.
cui_example(xmlui xml.c)
target_compile_definitions(xmlui PRIVATE CUI_XML_DEMO_PATH="${CMAKE_CURRENT_SOURCE_DIR}/xml_demo.xml")
