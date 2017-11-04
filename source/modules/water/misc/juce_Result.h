/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2016 - ROLI Ltd.

   Permission is granted to use this software under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license/

   Permission to use, copy, modify, and/or distribute this software for any
   purpose with or without fee is hereby granted, provided that the above
   copyright notice and this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH REGARD
   TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
   FITNESS. IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT,
   OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
   USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
   TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
   OF THIS SOFTWARE.

   -----------------------------------------------------------------------------

   To release a closed-source product which uses other parts of JUCE not
   licensed under the ISC terms, commercial licenses are available: visit
   www.juce.com for more information.

  ==============================================================================
*/

#ifndef JUCE_RESULT_H_INCLUDED
#define JUCE_RESULT_H_INCLUDED

namespace water {

//==============================================================================
/**
    Represents the 'success' or 'failure' of an operation, and holds an associated
    error message to describe the error when there's a failure.

    E.g.
    @code
    Result myOperation()
    {
        if (doSomeKindOfFoobar())
            return Result::ok();
        else
            return Result::fail ("foobar didn't work!");
    }

    const Result result (myOperation());

    if (result.wasOk())
    {
        ...it's all good...
    }
    else
    {
        warnUserAboutFailure ("The foobar operation failed! Error message was: "
                                + result.getErrorMessage());
    }
    @endcode
*/
class Result
{
public:
    //==============================================================================
    /** Creates and returns a 'successful' result. */
    static Result ok() noexcept                         { return Result(); }

    /** Creates a 'failure' result.
        If you pass a blank error message in here, a default "Unknown Error" message
        will be used instead.
    */
    static Result fail (const String& errorMessage) noexcept;

    //==============================================================================
    /** Returns true if this result indicates a success. */
    bool wasOk() const noexcept;

    /** Returns true if this result indicates a failure.
        You can use getErrorMessage() to retrieve the error message associated
        with the failure.
    */
    bool failed() const noexcept;

    /** Returns true if this result indicates a success.
        This is equivalent to calling wasOk().
    */
    operator bool() const noexcept;

    /** Returns true if this result indicates a failure.
        This is equivalent to calling failed().
    */
    bool operator!() const noexcept;

    /** Returns the error message that was set when this result was created.
        For a successful result, this will be an empty string;
    */
    const String& getErrorMessage() const noexcept;

    //==============================================================================
    Result (const Result&);
    Result& operator= (const Result&);

   #if JUCE_COMPILER_SUPPORTS_MOVE_SEMANTICS
    Result (Result&&) noexcept;
    Result& operator= (Result&&) noexcept;
   #endif

    bool operator== (const Result& other) const noexcept;
    bool operator!= (const Result& other) const noexcept;

private:
    String errorMessage;

    // The default constructor is not for public use!
    // Instead, use Result::ok() or Result::fail()
    Result() noexcept;
    explicit Result (const String&) noexcept;

    // These casts are private to prevent people trying to use the Result object in numeric contexts
    operator int() const;
    operator void*() const;
};

}

#endif   // JUCE_RESULT_H_INCLUDED
