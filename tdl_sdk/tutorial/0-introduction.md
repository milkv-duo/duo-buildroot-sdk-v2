# Introduction

This is the Cvitek TDL SDK. It can be used together with Cvitek produced cvimodels. The SDK is composed of four parts: core, evaluation, service, app.

## Core

The ``core`` library contains wrapped NN functions let users can easily integrate to their code base. THe core library includes the most classic models such as Yolov3.

## Evaluation

The ``evaluation`` library currently contains Coco, LFW, and Wider Face evaluation. This library helps users to evaluated their pre-trained module on boards.

## Service

The ``service`` module is composed of several of sub-modules. Currently only ``frservice`` and ``objservice`` are available.

## App

The ``app`` library is a solution designed based on TDL SDK for various client applications.

We provides some post-data analysis functions related to the provided networks, such as inner product for int8 feature matching, or digital tracking for detected results. Find out more details in chapter 3.
